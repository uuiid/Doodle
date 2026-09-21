# Deploy depth estimation worker from network share and supervise it.
#
# The script:
#   1. registers itself as a startup task (scheduled task: at startup + at logon)
#   2. deploys the worker binaries from the network share
#   3. starts the worker and registers it for depth_estimation tasks
#   4. keeps watching the worker and restarts it when it crashes
#      (-MaxRestarts attempts, -RestartIntervalMs between attempts)
#
# Usage:
#   .\deploy_depth_worker.ps1 [-InstallDir <path>] [-RunAsSystem]
#   .\deploy_depth_worker.ps1 -NoWatch              # deploy + start + exit (old behaviour)
#   .\deploy_depth_worker.ps1 -UnregisterStartup    # remove the startup registration
[CmdletBinding()]
param (
    [string]$InstallDir = "D:\depth_worker",
    [string]$TaskName = "Doodle-DepthWorker",
    [int]$MaxRestarts = 100,          # restart attempts after each crash
    [int]$RestartIntervalMs = 100,    # delay between two restart attempts
    [int]$ShareWaitSeconds = 300,     # how long to wait for the network share at boot
    [switch]$RunAsSystem,             # register the startup task for SYSTEM (no user logon needed)
    [switch]$NoWatch,                 # do not supervise, exit after the first successful start
    [switch]$NoStartupRegistration,   # leave the existing startup registration untouched
    [switch]$UnregisterStartup        # remove the startup registration and exit
)

$ErrorActionPreference = "Stop"

$ShareBin = "\\192.168.20.89\Doodle2\build\install\bin"
$ShareCuda = "\\192.168.20.89\Doodle2\build\cuda_dll"
$ExeName = "doodle_kitsu_supplement.exe"
$ProcName = [System.IO.Path]::GetFileNameWithoutExtension($ExeName)
$ExePath = Join-Path $InstallDir $ExeName
$ModelsDir = Join-Path $InstallDir "models\depth"
$RunKeyPath = "HKCU:\Software\Microsoft\Windows\CurrentVersion\Run"

# ---------------------------------------------------------------------------
# process helpers
# ---------------------------------------------------------------------------

function Get-WorkerProcess {
    Get-Process -Name $ProcName -ErrorAction SilentlyContinue |
        Where-Object { try { $_.Path -like "$InstallDir*" } catch { $false } }
}

function Test-WorkerAlive {
    param ($Worker)
    if ($null -eq $Worker) { return $false }
    if (-not $Worker.Process.HasExited) { return $true }
    # the cmd.exe wrapper can die while the worker survives; any live worker exe counts as alive
    return $null -ne (Get-WorkerProcess)
}

function Stop-Worker {
    param ($Worker)
    if ($null -ne $Worker) {
        Stop-Process -Id $Worker.Process.Id -Force -ErrorAction SilentlyContinue
    }
    Get-WorkerProcess | Stop-Process -Force -ErrorAction SilentlyContinue
    Start-Sleep -Milliseconds 300
}

# ExitCode is not always readable on a process object returned by Start-Process
function Get-ExitCodeText {
    param ($Process)
    try {
        $code = $Process.ExitCode
        if ($null -ne $code) { return $code }
    } catch { }
    return "unknown"
}

# Start the worker and wait until it reports its port, then register it on the server.
function Start-Worker {
    Stop-Worker -Worker $null

    # cmd.exe is used for the stdout redirect: Start-Process cannot read async output
    $stamp = Get-Date -Format "yyyyMMdd_HHmmss_fff"
    $stdoutFile = Join-Path $env:TEMP "depth_worker_stdout_$stamp.txt"
    $cmdArgs = "/c `"`"$ExePath`" --local > `"$stdoutFile`" 2>&1`""
    $proc = Start-Process -FilePath "cmd.exe" -ArgumentList $cmdArgs -PassThru -NoNewWindow -WorkingDirectory $InstallDir

    $port = $null
    $deadline = (Get-Date).AddSeconds(30)
    while ((Get-Date) -lt $deadline -and -not $proc.HasExited) {
        if (Test-Path $stdoutFile) {
            foreach ($line in (Get-Content -Path $stdoutFile -Tail 50)) {
                Write-Host "  $line"
                if ($null -eq $port -and $line -match '^\s*(\d{1,5})\s*$') {
                    $port = [int]$Matches[1]
                }
            }
        }
        if ($port) { break }
        Start-Sleep -Milliseconds 500
    }

    if ($proc.HasExited) { throw "Worker process exited prematurely (exit code $(Get-ExitCodeText $proc))" }
    if (-not $port) { throw "Could not determine worker port within 30 seconds" }

    Write-Host "Worker running on port $port (PID $($proc.Id))"
    Register-Worker -Port $port

    return [pscustomobject]@{ Process = $proc; Port = $port; StdoutFile = $stdoutFile }
}

# Start the worker, retrying on failure: -Attempts tries, -IntervalMs between two tries.
# Returns the worker object, or $null when every attempt failed.
function Start-WorkerWithRetry {
    param ([int]$Attempts, [int]$IntervalMs)

    for ($attempt = 1; $attempt -le $Attempts; $attempt++) {
        Start-Sleep -Milliseconds $IntervalMs
        try {
            return (Start-Worker)
        } catch {
            Write-Warning "Start attempt $attempt/$Attempts failed: $($_.Exception.Message)"
            Stop-Worker -Worker $null
        }
    }
    return $null
}

# Register for depth_estimation tasks only
function Register-Worker {
    param ([int]$Port)

    $body = @{ allowed_task_types = @("depth_estimation") } | ConvertTo-Json -Compress
    $url = "http://127.0.0.1:$Port/api/actions/local/task/run"
    Write-Host "POST $url $body"

    $attempt = 0
    do {
        try {
            $res = Invoke-WebRequest -Uri $url -Method Post -Body $body -ContentType "application/json" -TimeoutSec 10
            Write-Host "Worker registered: $($res.StatusCode) $($res.StatusDescription)"
            return
        } catch {
            $attempt++
            if ($attempt -ge 5) { throw }
            Write-Host "  Retry $attempt/5..."
            Start-Sleep -Seconds 2
        }
    } while ($true)
}

# ---------------------------------------------------------------------------
# startup registration (self registration)
# ---------------------------------------------------------------------------

function Install-StartupRegistration {
    if (-not $PSCommandPath) { throw "Cannot resolve the script path for the startup registration" }

    $arguments = "-NoProfile -ExecutionPolicy Bypass -File `"$PSCommandPath`" -InstallDir `"$InstallDir`" -NoStartupRegistration"
    if ($RunAsSystem) { $arguments += " -RunAsSystem" }

    try {
        $action = New-ScheduledTaskAction -Execute "powershell.exe" -Argument $arguments -WorkingDirectory $InstallDir
        $triggers = @(
            (New-ScheduledTaskTrigger -AtStartup)
            (New-ScheduledTaskTrigger -AtLogOn -User "$env:USERDOMAIN\$env:USERNAME")
        )
        # the watchdog runs forever, so the execution time limit must be unlimited
        $settings = New-ScheduledTaskSettingsSet -AllowStartIfOnBatteries -DontStopIfGoingOnBatteries `
            -StartWhenAvailable -MultipleInstances IgnoreNew -ExecutionTimeLimit ([TimeSpan]::Zero) `
            -RestartCount 3 -RestartInterval (New-TimeSpan -Minutes 1)
        if ($RunAsSystem) {
            $principal = New-ScheduledTaskPrincipal -UserId "SYSTEM" -LogonType ServiceAccount -RunLevel Highest
        } else {
            $principal = New-ScheduledTaskPrincipal -UserId "$env:USERDOMAIN\$env:USERNAME" -LogonType Interactive -RunLevel Highest
        }

        Register-ScheduledTask -TaskName $TaskName -Action $action -Trigger $triggers -Settings $settings `
            -Principal $principal -Description "Doodle depth estimation worker (deploy + watchdog)" -Force | Out-Null
        Write-Host "Startup registration: scheduled task '$TaskName' (at startup + at logon)"
    } catch {
        # no elevation: fall back to a per-user Run entry, which only covers logon
        Write-Warning "Scheduled task registration failed ($($_.Exception.Message)); using HKCU Run key instead"
        Set-ItemProperty -Path $RunKeyPath -Name $TaskName -Value "powershell.exe $arguments"
        Write-Host "Startup registration: HKCU Run key '$TaskName' (logon only)"
    }
}

function Remove-StartupRegistration {
    if (Get-ScheduledTask -TaskName $TaskName -ErrorAction SilentlyContinue) {
        Unregister-ScheduledTask -TaskName $TaskName -Confirm:$false
        Write-Host "Removed scheduled task '$TaskName'"
    }
    if (Get-ItemProperty -Path $RunKeyPath -Name $TaskName -ErrorAction SilentlyContinue) {
        Remove-ItemProperty -Path $RunKeyPath -Name $TaskName
        Write-Host "Removed HKCU Run entry '$TaskName'"
    }
}

# ---------------------------------------------------------------------------
# deployment
# ---------------------------------------------------------------------------

function Deploy-Worker {
    # 1. Create directories
    $null = New-Item -ItemType Directory -Force -Path $InstallDir
    $null = New-Item -ItemType Directory -Force -Path $ModelsDir

    # 2. Wait for the share: at boot the network may not be ready yet
    if (-not (Test-Path -LiteralPath $ShareBin)) {
        if ($ShareWaitSeconds -gt 0) {
            Write-Host "Waiting for $ShareBin (up to $ShareWaitSeconds s)..."
        }
        $deadline = (Get-Date).AddSeconds($ShareWaitSeconds)
        while ((Get-Date) -lt $deadline -and -not (Test-Path -LiteralPath $ShareBin)) {
            Start-Sleep -Seconds 5
        }
    }
    if (-not (Test-Path -LiteralPath $ShareBin)) {
        if (Test-Path -LiteralPath $ExePath) {
            Write-Warning "Share $ShareBin unreachable; keeping the binaries already in $InstallDir"
            return
        }
        throw "Share $ShareBin unreachable and no binaries in $InstallDir"
    }

    # 3. Copy runtime binaries from install share
    Write-Host "Copying runtime from $ShareBin ..."
    & robocopy $ShareBin $InstallDir /MIR /r:2 /w:1 /NDL /NFL
    if ($LASTEXITCODE -ge 8) { throw "robocopy runtime failed (exit $LASTEXITCODE)" }

    # 4. Copy CUDA runtime DLLs
    Write-Host "Copying CUDA DLLs from $ShareCuda ..."
    $cudaDlls = @("onnxruntime_providers_cuda.dll", "onnxruntime_providers_shared.dll")
    foreach ($dll in $cudaDlls) {
        $src = Join-Path $ShareCuda $dll
        if (Test-Path $src) {
            Copy-Item -Force $src $InstallDir
            Write-Host "  $dll"
        } else {
            Write-Warning "  $dll not found at $src"
        }
    }
}

# ---------------------------------------------------------------------------
# main
# ---------------------------------------------------------------------------

if ($UnregisterStartup) {
    Remove-StartupRegistration
    return
}

Write-Host "=== Deploying depth estimation worker ==="

if (-not $NoStartupRegistration) {
    Install-StartupRegistration
}

Deploy-Worker

$worker = Start-WorkerWithRetry -Attempts $MaxRestarts -IntervalMs $RestartIntervalMs
if ($null -eq $worker) {
    Write-Host "[$(Get-Date -Format 'HH:mm:ss')] Giving up: worker did not start after $MaxRestarts attempts" -ForegroundColor Red
    exit 1
}

Write-Host "=== Depth estimation worker deployed successfully ==="
Write-Host "PID: $($worker.Process.Id)  Port: $($worker.Port)  Dir: $InstallDir"
Write-Host "Log: $($worker.StdoutFile)"

if ($NoWatch) { return }

Write-Host "Watching worker (up to $MaxRestarts restart attempts, $RestartIntervalMs ms apart)..."

$restartTotal = 0
while ($true) {
    while (Test-WorkerAlive -Worker $worker) { Start-Sleep -Seconds 2 }

    Write-Host "[$(Get-Date -Format 'HH:mm:ss')] Worker exited (exit code $(Get-ExitCodeText $worker.Process))"

    $worker = Start-WorkerWithRetry -Attempts $MaxRestarts -IntervalMs $RestartIntervalMs
    if ($null -eq $worker) {
        Write-Host "[$(Get-Date -Format 'HH:mm:ss')] Giving up: worker not restarted after $MaxRestarts attempts" -ForegroundColor Red
        exit 1
    }

    $restartTotal++
    Write-Host "[$(Get-Date -Format 'HH:mm:ss')] Worker restarted on port $($worker.Port) (PID $($worker.Process.Id), total restarts $restartTotal)"
}
