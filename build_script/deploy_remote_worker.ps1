<#
.SYNOPSIS
    远程部署 Doodle 工作机：下载发布包 → 解压 → 启动 doodle_kitsu_supplement.exe → 注册分布式任务。

.DESCRIPTION
    通过 WinRM（PSSession）在目标机器上执行完整部署流程：
      1. 从固定 URL 下载发布包（默认 http://192.168.40.188/Doodle-3.6.1751-win64.zip）
      2. 解压，并把 doodle_kitsu_supplement.exe 所在目录镜像到安装目录（去掉旧版本残留）
      3. 以 --local 启动 doodle_kitsu_supplement.exe，从 stdout 解析实际监听端口
      4. POST http://127.0.0.1:<Port>/api/actions/local/task/run，body 默认 "{}"

    连接方式与 DoodlePackageFun.psm1 的 New-ServerPSSession 一致（IP + Basic 认证），
    目标机器需先执行过 build_script\enable_psremoting.ps1。

.PARAMETER ComputerName
    目标机器 IP 或主机名。

.PARAMETER UserName
    登录用户名，默认 administrator。

.PARAMETER Password
    登录密码（明文，仅限可信内网）。省略时交互式安全输入。

.PARAMETER Url
    发布包下载地址。

.PARAMETER InstallDir
    目标机器上的安装目录，默认 D:\doodle_worker。每次部署都会用 robocopy /MIR 镜像覆盖，
    并保留发布包内的目录结构（发布包是单层根目录 Doodle-3.6.1751-win64\{bin, maya}，
    因此 exe 落在 <InstallDir>\bin\doodle_kitsu_supplement.exe）。

.PARAMETER ExeName
    要启动的可执行文件名，默认 doodle_kitsu_supplement.exe。

.PARAMETER Port
    监听端口。0（默认）表示让系统分配，再从 stdout 解析。

.PARAMETER AllowedTaskTypes
    逗号分隔的任务类型列表。省略时 body 就是 "{}"，服务端此时按默认值
    {export_fbx, auto_light} 处理（见 http_method/local/event.cpp 的 actions_local_task_run post）。
    需要跑深度估计时传 -AllowedTaskTypes depth_estimation。

.PARAMETER PortWaitSeconds
    等待端口打印出来的超时秒数，默认 60。

.PARAMETER TaskRunRetry
    注册请求的重试次数，默认 5。

.PARAMETER OperationTimeoutMs
    WinRM 操作超时（毫秒），默认 3600000（1 小时）。下载大包会超过 180 秒的默认值。

.PARAMETER KeepArchive
    保留目标机器上的发布包与解压临时目录。

.EXAMPLE
    .\deploy_remote_worker.ps1 -ComputerName 192.168.40.190 -UserName administrator -Password 'sywh.888'

.EXAMPLE
    # 部署并只允许跑深度估计任务
    .\deploy_remote_worker.ps1 -ComputerName 192.168.40.190 -Password 'sywh.888' -AllowedTaskTypes depth_estimation

.EXAMPLE
    # 固定端口 + 保留安装包
    .\deploy_remote_worker.ps1 -ComputerName 192.168.40.190 -Password 'sywh.888' -Port 50025 -KeepArchive

.NOTES
    踩坑记录（改脚本前先看这里）：

    1) 远程启动的进程必须脱离 WinRM 会话的作业对象。Invoke-Command / Start-Process 起来的子进程
       属于 wsmprovhost.exe 的 job，会话一关（本脚本退出就会关）连同子进程一起被杀。
       所以这里用 Invoke-CimMethod Win32_Process Create 启动（父进程是 WmiPrvSE），
       并借 cmd.exe /c 自己完成 stdout 重定向 —— Win32_Process.Create 本身不支持重定向。

    2) $ErrorActionPreference = 'Stop' 时，原生命令往 stderr 写内容会被包装成终止错误
       NativeCommandFailed，直接把脚本打断（见 enable_psremoting.ps1 的同类记录）。
       远程块里所有原生命令（curl / robocopy）统一走 Invoke-Native，只取退出码。

    3) 端口是「就绪信号」：http_listener.cpp 在 acceptor.listen 之后会把实际端口按单独一行
       打到 stdout（std::cout << port << std::endl），所以必须解析 stdout 而不是假定端口。

    4) 必须先停掉正在运行的实例再覆盖文件，否则 exe/dll 被占用，解压和 robocopy 都会失败。

    5) 发布包只有一个顶层目录 Doodle-3.6.1751-win64，里面是 bin\(165 项, exe + 全部 DLL + dict)
       和 maya\(131 项, Maya 插件脚本)。镜像的是整棵包根而不是只镜像 bin\ —— 只镜像 bin\
       会丢掉 maya\，而默认任务类型(export_fbx / auto_light)是会用到 Maya 的。
       这也是 exe 不在安装目录根、而在 <InstallDir>\bin\ 下的原因。
#>
[CmdletBinding()]
param (
    [Parameter(Mandatory = $true, Position = 0)]
    [string]$ComputerName,

    [string]$UserName = "administrator",

    [string]$Password,

    [string]$Url = "http://192.168.40.188/Doodle-3.6.1751-win64.zip",

    [string]$InstallDir = "D:\doodle_worker",

    [string]$ExeName = "doodle_kitsu_supplement.exe",

    [int]$Port = 50025,

    [string]$AllowedTaskTypes,

    [int]$PortWaitSeconds = 60,

    [int]$TaskRunRetry = 5,

    [int]$OperationTimeoutMs = 3600000,

    [switch]$KeepArchive
)

$ErrorActionPreference = "Stop"

# ---------------------------------------------------------------------------
# 在目标机器上执行的部署脚本块（自包含，不依赖本机任何变量）
# ---------------------------------------------------------------------------
$RemoteScript = {
    param (
        [string]$Url,
        [string]$InstallDir,
        [string]$ExeName,
        [int]$Port,
        [string]$AllowedTaskTypes,
        [int]$PortWaitSeconds,
        [int]$TaskRunRetry,
        [bool]$KeepArchive
    )

    $ErrorActionPreference = "Stop"
    $ProgressPreference    = "SilentlyContinue"

    # $ErrorActionPreference = 'Stop' 下原生命令写 stderr 会变成终止错误，这里统一兜住，只取退出码
    function Invoke-Native {
        param([string]$FilePath, [string[]]$Arguments)
        $old = $ErrorActionPreference
        $ErrorActionPreference = "Continue"
        try {
            & $FilePath @Arguments 2>&1 | Out-Null
            return $LASTEXITCODE
        }
        finally {
            $ErrorActionPreference = $old
        }
    }

    $ProcName       = [System.IO.Path]::GetFileNameWithoutExtension($ExeName)
    $InstallDirFull = [System.IO.Path]::GetFullPath($InstallDir)
    $Stamp          = Get-Date -Format "yyyyMMdd_HHmmss"

    # ---- 1. 停掉正在运行的实例：否则 exe/dll 被占用，后面的覆盖一定失败 ----
    $running = @(Get-Process -Name $ProcName -ErrorAction SilentlyContinue |
            Where-Object { try { $_.Path -like "$InstallDirFull*" } catch { $false } })
    if ($running.Count -gt 0) {
        Write-Host "[1/5] 停止正在运行的 $ProcName (PID $($running.Id -join ', '))"
        $running | Stop-Process -Force -ErrorAction SilentlyContinue
        Start-Sleep -Milliseconds 800
    }
    else {
        Write-Host "[1/5] 没有正在运行的 $ProcName"
    }

    # ---- 2. 下载发布包 ----
    $ZipPath  = Join-Path $env:TEMP "doodle_package_$Stamp.zip"
    $StageDir = Join-Path $env:TEMP "doodle_stage_$Stamp"
    Write-Host "[2/5] 下载 $Url"
    if (Get-Command curl.exe -ErrorAction SilentlyContinue) {
        # curl.exe 比 Invoke-WebRequest 快得多（PS 5.1 的 Invoke-WebRequest 有进度条与内存问题）
        $code = Invoke-Native -FilePath "curl.exe" -Arguments @("-L", "--fail", "--silent", "--show-error", "-o", $ZipPath, $Url)
        if ($code -ne 0) { throw "下载失败 (curl exit $code): $Url" }
    }
    else {
        Invoke-WebRequest -Uri $Url -OutFile $ZipPath
    }
    if (-not (Test-Path -LiteralPath $ZipPath)) { throw "下载失败, 文件不存在: $Url" }
    $sizeMb = [math]::Round((Get-Item -LiteralPath $ZipPath).Length / 1MB, 1)
    Write-Host "      下载完成: $sizeMb MB -> $ZipPath"

    # ---- 3. 解压并镜像到安装目录 ----
    if (Test-Path -LiteralPath $StageDir) { Remove-Item -LiteralPath $StageDir -Recurse -Force }
    $null = New-Item -ItemType Directory -Force -Path $StageDir
    Write-Host "[3/5] 解压到 $StageDir"
    Add-Type -AssemblyName System.IO.Compression.FileSystem
    [System.IO.Compression.ZipFile]::ExtractToDirectory($ZipPath, $StageDir)

    # 发布包是单层根目录: Doodle-3.6.1751-win64\{bin, maya}, exe 在 bin\ 下。
    # 这里镜像整棵包根而不是只镜像 bin\, 否则会丢掉 maya 这类运行期要用的同级目录
    $topDirs = @(Get-ChildItem -LiteralPath $StageDir -Directory)
    $SrcRoot = if ($topDirs.Count -eq 1) { $topDirs[0].FullName } else { $StageDir }
    Write-Host "      发布根目录: $SrcRoot"

    $null = New-Item -ItemType Directory -Force -Path $InstallDirFull
    $code = Invoke-Native -FilePath "robocopy" `
        -Arguments @($SrcRoot, $InstallDirFull, "/MIR", "/r:2", "/w:1", "/NDL", "/NFL", "/NJH", "/NJS")
    if ($code -ge 8) { throw "robocopy 镜像失败 (exit $code): $SrcRoot -> $InstallDirFull" }

    # 按最短路径定位 exe 的实际位置(包内是 bin\)
    $exeFile = Get-ChildItem -LiteralPath $InstallDirFull -Filter $ExeName -Recurse -File -ErrorAction SilentlyContinue |
        Sort-Object { $_.FullName.Length } | Select-Object -First 1
    if (-not $exeFile) { throw "镜像后没有在 $InstallDirFull 下找到 $ExeName" }
    $ExePath = $exeFile.FullName
    $WorkDir = $exeFile.DirectoryName
    Write-Host "      已部署到 $InstallDirFull (robocopy exit $code)"
    Write-Host "      程序: $ExePath"

    # ---- 4. 启动并等待端口 ----
    $StdoutFile = Join-Path $env:TEMP "doodle_supplement_$Stamp.log"
    $exeArgs    = "--local"
    if ($Port -gt 0) { $exeArgs = "$exeArgs --port $Port" }

    # cmd.exe /c 负责 stdout 重定向; WMI 负责让进程脱离 WinRM 会话(见文件头 NOTES 第 1 条)
    $cmdLine = "cmd.exe /c `"`"$ExePath`" $exeArgs > `"$StdoutFile`" 2>&1`""
    Write-Host "[4/5] 启动: $ExePath $exeArgs"
    Write-Host "      日志: $StdoutFile"
    $created = Invoke-CimMethod -ClassName Win32_Process -MethodName Create -Arguments @{
        CommandLine      = $cmdLine
        CurrentDirectory = $WorkDir
    }
    if ($created.ReturnValue -ne 0) { throw "启动失败 (Win32_Process.Create ReturnValue=$($created.ReturnValue))" }

    $ListenPort = 0
    $deadline   = (Get-Date).AddSeconds($PortWaitSeconds)
    while ((Get-Date) -lt $deadline) {
        if (Test-Path -LiteralPath $StdoutFile) {
            $lines = @(Get-Content -LiteralPath $StdoutFile -ErrorAction SilentlyContinue)
            foreach ($line in $lines) {
                # 监听器就绪时会把实际端口单独打一行
                if ($line -match '^\s*(\d{1,5})\s*$') { $ListenPort = [int]$Matches[1]; break }
            }
        }
        if ($ListenPort -gt 0) { break }
        if (-not (Get-Process -Name $ProcName -ErrorAction SilentlyContinue)) {
            $tail = if (Test-Path -LiteralPath $StdoutFile) { (Get-Content -LiteralPath $StdoutFile -Tail 20) -join "`n" } else { "(无日志)" }
            throw "进程已退出, 日志尾部:`n$tail"
        }
        Start-Sleep -Milliseconds 500
    }
    if ($ListenPort -le 0) {
        $tail = if (Test-Path -LiteralPath $StdoutFile) { (Get-Content -LiteralPath $StdoutFile -Tail 20) -join "`n" } else { "(无日志)" }
        throw "等待 $PortWaitSeconds 秒仍未解析到监听端口, 日志尾部:`n$tail"
    }
    Write-Host "      监听端口: $ListenPort"

    # ---- 5. 注册分布式任务 ----
    $body = "{}"
    if ($AllowedTaskTypes) {
        $types = @($AllowedTaskTypes -split "," | ForEach-Object { $_.Trim() } | Where-Object { $_ })
        if ($types.Count -gt 0) { $body = @{ allowed_task_types = $types } | ConvertTo-Json -Compress }
    }
    $runUrl = "http://127.0.0.1:$ListenPort/api/actions/local/task/run"
    Write-Host "[5/5] POST $runUrl  body: $body"

    $posted = $false
    for ($i = 1; $i -le $TaskRunRetry; $i++) {
        try {
            # 5.1 需要 -UseBasicParsing(无 IE 引擎), 与 deployment_server.ps1 的处理保持一致
            if ($PSVersionTable.PSVersion.Major -lt 6) {
                $res = Invoke-WebRequest -Uri $runUrl -Method Post -Body $body -ContentType "application/json" `
                    -TimeoutSec 15 -UseBasicParsing
            }
            else {
                $res = Invoke-WebRequest -Uri $runUrl -Method Post -Body $body -ContentType "application/json" -TimeoutSec 15
            }
            Write-Host "      注册成功: HTTP $($res.StatusCode)"
            $posted = $true
            break
        }
        catch {
            if ($i -ge $TaskRunRetry) { throw "注册失败($TaskRunRetry 次): $($_.Exception.Message)" }
            Write-Host "      第 $i 次失败, 重试... ($($_.Exception.Message))"
            Start-Sleep -Seconds 2
        }
    }
    if (-not $posted) { throw "注册失败: $runUrl" }

    # ---- 清理 ----
    if (-not $KeepArchive) {
        Remove-Item -LiteralPath $StageDir -Recurse -Force -ErrorAction SilentlyContinue
        Remove-Item -LiteralPath $ZipPath -Force -ErrorAction SilentlyContinue
    }

    return [pscustomobject]@{
        ComputerName = $env:COMPUTERNAME
        ExePath      = $ExePath
        WorkDir      = $WorkDir
        InstallDir   = $InstallDirFull
        Port         = $ListenPort
        RunUrl       = $runUrl
        Body         = $body
        StdoutFile   = $StdoutFile
        ZipPath      = if ($KeepArchive) { $ZipPath } else { "" }
    }
}

# ---------------------------------------------------------------------------
# 主流程
# ---------------------------------------------------------------------------
Write-Host ""
Write-Host "==== 远程部署 Doodle 工作机 ====" -ForegroundColor Cyan
Write-Host "目标    : $ComputerName (用户 $UserName)"
Write-Host "发布包  : $Url"
Write-Host "安装目录: $InstallDir"
Write-Host ""

if (-not $Password) {
    $securePassword = Read-Host -Prompt "请输入 $UserName@$ComputerName 的密码" -AsSecureString
}
else {
    $securePassword = ConvertTo-SecureString $Password -AsPlainText -Force
}
$credential = New-Object System.Management.Automation.PSCredential($UserName, $securePassword)

$sessionOption = New-PSSessionOption -OperationTimeout $OperationTimeoutMs
$session       = $null
try {
    Write-Host "[连接] 建立 PSSession..."
    try {
        # 与 DoodlePackageFun.psm1 的 New-ServerPSSession 一致
        $session = New-PSSession -ComputerName $ComputerName -Credential $credential -Authentication Basic `
            -SessionOption $sessionOption -ErrorAction Stop
    }
    catch {
        Write-Warning "Basic 认证失败($($_.Exception.Message)), 改用默认(Negotiate)重试"
        $session = New-PSSession -ComputerName $ComputerName -Credential $credential `
            -SessionOption $sessionOption -ErrorAction Stop
    }
    Write-Host "[连接] 已连接 $ComputerName"
    Write-Host ""

    $info = Invoke-Command -Session $session -ScriptBlock $RemoteScript -ArgumentList @(
        $Url, $InstallDir, $ExeName, $Port, $AllowedTaskTypes, $PortWaitSeconds, $TaskRunRetry, [bool]$KeepArchive
    )

    Write-Host ""
    Write-Host "==== 部署完成 ====" -ForegroundColor Green
    Write-Host "机器    : $($info.ComputerName) ($ComputerName)"
    Write-Host "安装目录: $($info.InstallDir)"
    Write-Host "程序    : $($info.ExePath)"
    Write-Host "工作目录: $($info.WorkDir)"
    Write-Host "端口    : $($info.Port)"
    Write-Host "注册地址: $($info.RunUrl)"
    Write-Host "注册 body: $($info.Body)"
    Write-Host "日志    : $($info.StdoutFile)"
    if ($info.ZipPath) { Write-Host "安装包  : $($info.ZipPath)" }
    Write-Host ""
    Write-Host "停止分布式任务: Invoke-WebRequest -Uri '$($info.RunUrl)' -Method Delete" -ForegroundColor Gray
    Write-Host "查看是否在跑  : Invoke-WebRequest -Uri '$($info.RunUrl)' -Method Get" -ForegroundColor Gray
    Write-Host ""
}
finally {
    if ($session) {
        Remove-PSSession -Session $session -ErrorAction SilentlyContinue
        Write-Host "[连接] PSSession 已关闭（远程进程不受影响）" -ForegroundColor Gray
    }
}
