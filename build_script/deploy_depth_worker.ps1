# Deploy depth estimation worker from network share
# Usage: .\deploy_depth_worker.ps1 [-InstallDir <path>]
param (
    [string]$InstallDir = "D:\depth_worker"
)

$ErrorActionPreference = "Stop"

$ShareBin    = "\\192.168.20.89\Doodle2\build\install\bin"
$ShareCuda   = "\\192.168.20.89\Doodle2\build\cuda_dll"
$ExeName     = "doodle_kitsu_supplement.exe"
$ModelsDir   = Join-Path $InstallDir "models\depth"

Write-Host "=== Deploying depth estimation worker ==="

# 1. Create directories
$null = New-Item -ItemType Directory -Force -Path $InstallDir
$null = New-Item -ItemType Directory -Force -Path $ModelsDir

# 2. Copy runtime binaries from install share
Write-Host "Copying runtime from $ShareBin ..."
& robocopy $ShareBin $InstallDir /MIR /w:1 /NDL /NFL
if ($LASTEXITCODE -ge 8) { throw "robocopy runtime failed (exit $LASTEXITCODE)" }

# 3. Copy CUDA runtime DLLs
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

# 4. Kill any stale instance
Get-Process -Name "doodle_kitsu_supplement" -ErrorAction SilentlyContinue |
    Where-Object { $_.Path -like "$InstallDir*" } |
    Stop-Process -Force

# 5. Start worker with --local, capture port from stdout via temp file
Write-Host "Starting worker..."
$stdoutFile = Join-Path $env:TEMP "depth_worker_stdout_$pid.txt"
if (Test-Path $stdoutFile) { Remove-Item $stdoutFile -Force }

# Use cmd.exe to redirect stdout since PowerShell Start-Process has issues with async IO
$exePath   = Join-Path $InstallDir $ExeName
$cmdArgs   = "/c `"`"$exePath`" --local > `"$stdoutFile`" 2>&1`""
$proc      = Start-Process -FilePath "cmd.exe" -ArgumentList $cmdArgs -PassThru -NoNewWindow -WorkingDirectory $InstallDir

# 6. Poll stdout file for port number
$port      = $null
$deadline  = (Get-Date).AddSeconds(30)
$seekPos   = 0

while ((Get-Date) -lt $deadline -and !$proc.HasExited) {
    if (Test-Path $stdoutFile) {
        $newContent = Get-Content -Path $stdoutFile -Tail 50
        foreach ($line in $newContent) {
            Write-Host "  $line"
            if ($port -eq $null -and $line -match '^\s*(\d{1,5})\s*$') {
                $port = [int]$Matches[1]
            }
        }
    }
    if ($port) { break }
    Start-Sleep -Milliseconds 500
}

if ($proc.HasExited) {
    throw "Worker process exited prematurely with code $($proc.ExitCode)"
}
if (-not $port) {
    throw "Could not determine worker port within 30 seconds"
}

Write-Host "Worker running on port $port"

# 7. Register for depth_estimation tasks only
$body = @{ allowed_task_types = @("depth_estimation") } | ConvertTo-Json -Compress
$url  = "http://127.0.0.1:$port/api/actions/local/task/run"
Write-Host "POST $url $body"

$attempt = 0
do {
    try {
        $res = Invoke-WebRequest -Uri $url -Method Post -Body $body -ContentType "application/json" -TimeoutSec 10
        Write-Host "Worker registered: $($res.StatusCode) $($res.StatusDescription)"
        break
    } catch {
        $attempt++
        if ($attempt -ge 5) { throw $_ }
        Write-Host "  Retry $attempt/5..."
        Start-Sleep -Seconds 2
    }
} while ($true)

Write-Host "=== Depth estimation worker deployed successfully ==="
Write-Host "PID: $($proc.Id)  Port: $port  Dir: $InstallDir"
Write-Host "Log: $stdoutFile"