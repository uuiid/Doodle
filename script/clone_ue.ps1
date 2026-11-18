# 弱网下把已存在的 UnrealEngine 工作仓库从浅克隆/部分克隆补成完整历史。
# 用法：
#   pwsh -File script\clone_ue.ps1                 # 从断点续跑，直到完成或重试耗尽
#   pwsh -File script\clone_ue.ps1 -Status         # 只看当前进度
#   pwsh -File script\clone_ue.ps1 -Reset          # 清空进度记录后重来
# 先确认 UnrealEngine 目录可写：脚本会更新 .git 内的 FETCH_HEAD / refs / objects，
# 权限不足时 git 会报 `cannot open .git/FETCH_HEAD: Permission denied`。
# 需要的实际提交由「已完成窗口」状态文件驱动，中途失败直接用同一条命令重跑即可。
# 状态文件（done.txt / clone_ue.log）写在 build\clone_ue_state，不进 git 仓库。

[CmdletBinding()]
param(
    # 每个分块窗口取多少个提交；UE 单分支约 35 万提交，1 万一份大约 35 份
    [int]$WindowSize = 10000,
    # 单次 git 命令的网络上限（秒）；超时即按失败处理、写断点退出
    [int]$FetchTimeoutSec = 1800,
    # 单个窗口失败后的重试次数
    [int]$MaxRetry = 10,
    # 首次重试等待秒数，之后指数退避，上限 RetryWaitMax
    [int]$RetryWaitSec = 10,
    [int]$RetryWaitMaxSec = 600,
    # 只补提交历史，不下载文件内容（blob）；真正需要 blob 时再跑 git backfill
    [switch]$SkipBackfill,
    # 只打印计划，不执行任何 fetch
    [switch]$DryRun,
    # 打印进度与仓库状态后退出
    [switch]$Status,
    # 清空状态目录（对象库不会回退，已下载的内容保留）
    [switch]$Reset,
    # 新克隆时使用的上游地址
    [string]$RemoteUrl = "git@github.com:EpicGames/UnrealEngine.git",
    # 目标分支
    [string]$Branch = "5.7"
)

$ErrorActionPreference = "Stop"
$Repo = (Resolve-Path (Join-Path $PSScriptRoot "..\UnrealEngine")).Path
$StateDir = Join-Path $PSScriptRoot "..\build\clone_ue_state"
$DoneFile = Join-Path $StateDir "done.txt"
$LogFile = Join-Path $StateDir "clone_ue.log"

function Write-Log {
    param([string]$Message, [string]$Level = "INFO")
    $line = "{0} [{1}] {2}" -f (Get-Date -Format "yyyy-MM-dd HH:mm:ss"), $Level, $Message
    Write-Host $line
    if (Test-Path $StateDir) {
        Add-Content -Path $LogFile -Value $line -Encoding utf8
    }
}

# 所有 git 都经这里走：固定 UTF-8、禁用交互提示、可超时，返回输出与退出码
function Invoke-Git {
    param(
        [string[]]$Arguments,
        [int]$TimeoutSec = 0,
        [switch]$AllowFail
    )
    $psi = [System.Diagnostics.ProcessStartInfo]::new()
    $psi.FileName = "git"
    foreach ($a in $Arguments) { $psi.ArgumentList.Add($a) }
    $psi.WorkingDirectory = $Repo
    $psi.RedirectStandardOutput = $true
    $psi.RedirectStandardError = $true
    $psi.UseShellExecute = $false
    $psi.StandardOutputEncoding = [System.Text.UTF8Encoding]::new($false)
    $psi.Environment["GIT_TERMINAL_PROMPT"] = "0"
    # 沿用环境里已设置的 GIT_SSH_COMMAND（例如指定 OpenSSH 绝对路径），只在末尾追加保活参数
    $sshBase = if ($env:GIT_SSH_COMMAND) { $env:GIT_SSH_COMMAND } else { "ssh" }
    $psi.Environment["GIT_SSH_COMMAND"] = "$sshBase -o ServerAliveInterval=30 -o ServerAliveCountMax=6"
    $psi.Environment["LC_ALL"] = "C"
    $p = [System.Diagnostics.Process]::Start($psi)
    $outTask = $p.StandardOutput.ReadToEndAsync()
    $errTask = $p.StandardError.ReadToEndAsync()
    $limitMs = if ($TimeoutSec -gt 0) { [int][Math]::Min([int64]$TimeoutSec * 1000, [int64][int]::MaxValue) } else { [int]::MaxValue }
    if (-not $p.WaitForExit($limitMs)) {
        try { $p.Kill($true) } catch { }
        Write-Log "git $($Arguments -join ' ') 超过 ${TimeoutSec}s，已终止" "WARN"
        if (-not $AllowFail) { throw "git 超时: $($Arguments -join ' ')" }
        return @{ Code = -1; Out = ""; Err = "timeout" }
    }
    $p.WaitForExit()
    $out = $outTask.Result
    $err = $errTask.Result
    if ($p.ExitCode -ne 0 -and -not $AllowFail) {
        throw ("git 失败 (exit {0}): git {1}`n{2}{3}" -f $p.ExitCode, ($Arguments -join " "), $out, $err)
    }
    return @{ Code = $p.ExitCode; Out = $out; Err = $err }
}

function Get-GitText {
    param([string[]]$Arguments, [switch]$AllowFail)
    $r = Invoke-Git -Arguments $Arguments -AllowFail:$AllowFail
    return $r.Out.Trim()
}

function Test-Git {
    param([string[]]$Arguments)
    return (Invoke-Git -Arguments $Arguments -AllowFail).Code -eq 0
}

function Get-ShallowCount {
    $p = Join-Path $Repo ".git\shallow"
    if (-not (Test-Path $p)) { return 0 }
    return (Get-Content -Path $p).Count
}

function Get-CommitCount {
    param([string]$Rev)
    $n = Get-GitText -Arguments @("rev-list", "--count", $Rev) -AllowFail
    if (-not $n) { return 0 }
    return [int]$n
}

function Show-Status {
    if (-not (Test-Path (Join-Path $Repo ".git"))) {
        Write-Host "仓库不存在: $Repo"
        return
    }
    $depth = Get-CommitCount $Branch
    Write-Host "仓库      : $Repo"
    Write-Host "分支      : $Branch"
    Write-Host "提交深度  : $depth"
    Write-Host "shallow   : $(Get-GitText -Arguments @('rev-parse', '--is-shallow-repository'))"
    Write-Host "边界数    : $(Get-ShallowCount)"
    Write-Host "部分克隆  : $(Get-GitText -Arguments @('config', '--get', 'remote.origin.partialclonefilter') -AllowFail)"
    if (Test-Path $DoneFile) {
        $done = @(Get-Content -Path $DoneFile | Where-Object { $_ -and -not $_.StartsWith("#") })
        Write-Host "已完成窗口: $($done.Count)"
        if ($done.Count -gt 0) { Write-Host "最近窗口  : $($done[-1])" }
    }
    else {
        Write-Host "已完成窗口: 0（尚无状态文件）"
    }
}

function Initialize-State {
    if ($Reset -and (Test-Path $StateDir)) {
        Remove-Item -Path $DoneFile -ErrorAction SilentlyContinue
        Write-Log "已清空进度记录（对象库与 refs 不受影响）" "WARN"
    }
    if (-not (Test-Path $StateDir)) {
        New-Item -ItemType Directory -Path $StateDir | Out-Null
    }
    if (-not (Test-Path $DoneFile)) {
        Set-Content -Path $DoneFile -Value "# clone_ue 进度记录：每行一个已完成的 deepen 窗口深度（git rev-list --count 目标分支）" -Encoding utf8
    }
}

function Get-CompletedDepths {
    if (-not (Test-Path $DoneFile)) { return @() }
    return @(Get-Content -Path $DoneFile | Where-Object { $_ -and -not $_.StartsWith("#") } | ForEach-Object { [int]$_ })
}

function Add-CompletedDepth {
    param([int]$Depth)
    Add-Content -Path $DoneFile -Value $Depth -Encoding utf8
}

# 幂等地确保仓库与上游配置存在（全新目录时等价于一次单分支浅克隆）
function Initialize-Repo {
    if (-not (Test-Path (Join-Path $Repo ".git"))) {
        Write-Log "仓库不存在，执行单分支浅克隆到 $Repo"
        New-Item -ItemType Directory -Path $Repo -Force | Out-Null
        $null = Invoke-Git -Arguments @("init") -AllowFail
    }
    $cur = Get-GitText -Arguments @("remote", "get-url", "origin") -AllowFail
    if (-not $cur) {
        Write-Log "配置 origin = $RemoteUrl"
        $null = Invoke-Git -Arguments @("remote", "add", "origin", $RemoteUrl)
    }
    elseif ($cur -ne $RemoteUrl) {
        Write-Log "origin 当前为 $cur，脚本期望 $RemoteUrl；沿用现有 origin，不覆盖" "WARN"
    }

    $hasRef = Test-Git -Arguments @("rev-parse", "--verify", "--quiet", "refs/heads/$Branch")
    $hasRemoteRef = Test-Git -Arguments @("rev-parse", "--verify", "--quiet", "refs/remotes/origin/$Branch")
    if (-not $hasRef) {
        Write-Log "首次拉取 $Branch 的 $WindowSize 个提交"
        $null = Invoke-Git -Arguments @(
            "fetch", "--progress", "--no-tags", "--depth=$WindowSize",
            "origin", "+refs/heads/${Branch}:refs/remotes/origin/$Branch",
            "+refs/heads/${Branch}:refs/heads/$Branch"
        ) -TimeoutSec $FetchTimeoutSec
        $null = Invoke-Git -Arguments @("config", "branch.$Branch.remote", "origin")
        $null = Invoke-Git -Arguments @("config", "branch.$Branch.merge", "refs/heads/$Branch")
        $null = Invoke-Git -Arguments @("checkout", "-f", $Branch) -AllowFail
        Add-CompletedDepth $WindowSize
        return
    }
    if (-not $hasRemoteRef) {
        $null = Invoke-Git -Arguments @("update-ref", "refs/remotes/origin/$Branch", "refs/heads/$Branch")
    }
}

# 一次窗口：在当前深度上加 WindowSize。--depth 是绝对值，所以要用 rev-list 算出的目标深度
function Invoke-Window {
    param([int]$Target)
    $depthArg = "--depth=$Target"
    $r = Invoke-Git -Arguments @(
        "fetch", "--progress", "--no-tags", $depthArg,
        "origin", "+refs/heads/${Branch}:refs/remotes/origin/$Branch"
    ) -TimeoutSec $FetchTimeoutSec -AllowFail
    if ($r.Code -ne 0) {
        $tail = ($r.Err -split "`r?`n" | Select-Object -Last 5) -join " | "
        Write-Log "窗口 depth=$Target 失败 (exit $($r.Code)): $tail" "WARN"
        return $false
    }
    Add-CompletedDepth $Target
    Write-Log "窗口完成：深度 $(Get-CommitCount $Branch)（记录 $Target）"
    return $true
}

# ---------- main ----------
if ($Status) { Show-Status; exit 0 }

Initialize-State
Write-Log "===== clone_ue 开始 ====="
Initialize-Repo

if ($DryRun) {
    $cur = Get-CommitCount $Branch
    Write-Log "[DryRun] 当前深度 $cur，shallow=$(Get-GitText -Arguments @('rev-parse', '--is-shallow-repository'))，下一次窗口 depth=$($cur + $WindowSize)"
    Show-Status
    exit 0
}

if (Test-Path (Join-Path $Repo ".git\index.lock")) {
    Write-Log "存在 .git\index.lock，可能另一个 git 进程在运行，退出以免破坏仓库" "ERROR"
    exit 2
}

$round = 0
while ($true) {
    $shallow = Get-GitText -Arguments @("rev-parse", "--is-shallow-repository") -AllowFail
    if ($shallow -ne "true") {
        Write-Log "仓库已完整（非浅克隆）"
        break
    }
    $cur = Get-CommitCount $Branch
    if ($cur -eq 0) {
        Write-Log "无法计算 $Branch 深度，仓库状态异常" "ERROR"
        exit 3
    }
    $target = $cur + $WindowSize
    $round++
    Write-Log "===== 第 $round 轮：depth $cur -> $target ====="

    $ok = $false
    for ($try = 1; $try -le $MaxRetry; $try++) {
        if (Invoke-Window -Target $target) { $ok = $true; break }
        if ($try -lt $MaxRetry) {
            $wait = [Math]::Min($RetryWaitSec * [Math]::Pow(2, $try - 1), $RetryWaitMaxSec)
            Write-Log "第 $try 次失败，${wait}s 后重试" "WARN"
            Start-Sleep -Seconds $wait
        }
    }
    if (-not $ok) {
        Write-Log "连续 $MaxRetry 次失败。断点：深度 $cur，目标 $target。稍后用同一条命令重跑即可继续。" "ERROR"
        Show-Status
        exit 1
    }

    if ((Get-CommitCount $Branch) -le $cur) {
        Write-Log "depth=$target 已成功但深度没有增长，可能是上游历史已到底或 fetch 未生效，退出以免空转" "ERROR"
        Show-Status
        exit 4
    }
}

# 历史完整后：拉 tag，并把 partial clone 的 blob 补齐（可选，耗时最长）
if (-not $SkipBackfill) {
    Write-Log "补齐 tag"
    $null = Invoke-Git -Arguments @("fetch", "--progress", "--tags", "origin") -TimeoutSec $FetchTimeoutSec -AllowFail
    Write-Log "backfill blob（--no-sparse，补全部缺失 blob）"
    $r = Invoke-Git -Arguments @("backfill", "--no-sparse", "--min-batch-size=100") -TimeoutSec $FetchTimeoutSec -AllowFail
    if ($r.Code -ne 0) {
        Write-Log "backfill 未完成 (exit $($r.Code))，可重跑本脚本或单独执行: git backfill --no-sparse；shallow 已消除，不影响历史完整性" "WARN"
    }
}

Write-Log "===== clone_ue 结束 ====="
Show-Status
