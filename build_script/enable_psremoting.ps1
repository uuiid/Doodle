<#
.SYNOPSIS
    开启本机 PowerShell 远程访问（WinRM / PSRemoting）权限，允许通过 IP 地址连接，
    并把 PowerShell 脚本执行策略调整为可运行本地脚本。

.DESCRIPTION
    对应 DoodlePackageFun.psm1 中 New-ServerPSSession 的连接方式：
        New-PSSession -ComputerName <IP> -Credential <administrator> -Authentication Basic

    非域环境下「IP + Basic 认证」连接需要同时满足两侧条件：
      服务端：WinRM 服务自启 + HTTP 监听器(5985, Address=*) + 防火墙放行
              + Service\Auth\Basic=true + Service\AllowUnencrypted=true
      客户端：Client\TrustedHosts 包含目标 IP
              + Client\Auth\Basic=true + Client\AllowUnencrypted=true

    本脚本默认同时配置服务端与客户端（-Mode Both），需要管理员权限，
    非管理员时会自动以管理员身份重新启动（用 -NoElevate 可关闭该行为）。

.PARAMETER Mode
    Both（默认）/ Server / Client。
    Server = 让别的机器能连本机；Client = 让本机能连别的机器。

.PARAMETER TrustedHosts
    客户端信任的主机列表，支持通配符，默认 '*' 表示信任全部。
    用 IP 连接时该列表必须包含目标 IP（非域环境必填，否则报「WinRM 客户端无法处理该请求」）。

.PARAMETER ExecutionPolicy
    要写入 CurrentUser / LocalMachine 的执行策略，默认 RemoteSigned。
    当前进程内固定为 Bypass，不影响系统设置。

.PARAMETER SkipExecutionPolicy
    跳过执行策略调整。

.PARAMETER Port
    WinRM HTTP 端口，默认 5985。

.PARAMETER AllowAnyNetworkProfile
    默认 $true：即使网络位置是「公用」也放行（Enable-PSRemoting -SkipNetworkProfileCheck）。

.PARAMETER EnableCredSSP
    额外开启 CredSSP，用于远程会话中做二次跳转（例如在远程会话里访问 UNC 共享）。

.PARAMETER NoElevate
    没有管理员权限时直接报错退出，不自动提权。

.PARAMETER SkipVerify
    跳过最后的连通性自检。

.EXAMPLE
    # 推荐：在目标机器上执行，本机同时作为服务端和客户端，全部放行
    powershell -ExecutionPolicy Bypass -File .\build_script\enable_psremoting.ps1
    powershell -ExecutionPolicy Bypass -File \\192.168.20.89\Doodle2\build_script\enable_psremoting.ps1 -EnableCredSSP

.EXAMPLE
    # 只让本机可以被访问，且只信任指定地址
    .\enable_psremoting.ps1 -Mode Server -TrustedHosts '192.168.40.188'

.EXAMPLE
    # 客户端侧：只配置「能连别人」，放行整个网段
    .\enable_psremoting.ps1 -Mode Client -TrustedHosts '192.168.40.*'

.NOTES
    安全提示：Basic + AllowUnencrypted 会让账号密码以明文经 HTTP(5985) 传输，
    仅建议在可信内网使用。跨网段或生产环境请改用 HTTPS 监听器(5986)、
    CredSSP 或证书认证，并把 -TrustedHosts 收窄到具体 IP 而不是 '*'。
#>
[CmdletBinding()]
param(
    [ValidateSet('Both', 'Server', 'Client')]
    [string]$Mode = 'Both',

    [string[]]$TrustedHosts = @('*'),

    [ValidateSet('Restricted', 'AllSigned', 'RemoteSigned', 'Unrestricted', 'Bypass')]
    [string]$ExecutionPolicy = 'RemoteSigned',

    [switch]$SkipExecutionPolicy,

    [int]$Port = 5985,

    [bool]$AllowAnyNetworkProfile = $true,

    [switch]$EnableCredSSP,

    [switch]$NoElevate,

    [switch]$SkipVerify
)

$ErrorActionPreference = 'Stop'

# ---------------------------------------------------------------- 工具函数

function Test-IsAdmin {
    $identity = [Security.Principal.WindowsIdentity]::GetCurrent()
    $principal = New-Object Security.Principal.WindowsPrincipal($identity)
    return $principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
}

# 5.1 与 7 的执行策略分开存储，WinRM 远程会话默认跑 5.1，所以两个宿主都要设置
function Set-HostExecutionPolicy {
    param(
        [string]$HostExe,
        [string]$Policy
    )
    $command = Get-Command $HostExe -ErrorAction SilentlyContinue
    if (-not $command) {
        Write-Host "  [跳过] 未找到 $HostExe（该版本 PowerShell 未安装）"
        return
    }
    # 子进程自身用 -ExecutionPolicy Bypass 启动，避免被现有策略挡住
    $inner = 'foreach ($s in @("CurrentUser","LocalMachine")) { try { Set-ExecutionPolicy -ExecutionPolicy ' +
    $Policy + ' -Scope $s -Force -ErrorAction Stop } catch {} }'
    & $command.Source -NoProfile -ExecutionPolicy Bypass -Command $inner 2>&1 | Out-Null
    $result = & $command.Source -NoProfile -ExecutionPolicy Bypass -Command 'Get-ExecutionPolicy -Scope LocalMachine' 2>$null
    Write-Host "  [OK] ${HostExe}: LocalMachine = $result"
}

function Add-TrustedHost {
    param([string[]]$Hosts)

    $path = 'WSMan:\localhost\Client\TrustedHosts'
    $current = (Get-Item -Path $path -ErrorAction SilentlyContinue).Value
    $list = @()
    if ($current) {
        $list = @($current -split ',' | ForEach-Object { $_.Trim() } | Where-Object { $_ })
    }
    if ($list -contains '*') {
        Write-Host "  [OK] TrustedHosts 已为 '*'，无需追加"
        return
    }

    $added = @()
    foreach ($item in $Hosts) {
        if ($list -notcontains $item) {
            $list += $item
            $added += $item
        }
    }
    if ($added.Count -eq 0) {
        Write-Host "  [OK] TrustedHosts 已包含: $($Hosts -join ', ')"
        return
    }
    Set-Item -Path $path -Value ($list -join ',') -Force
    Write-Host "  [OK] TrustedHosts = $($list -join ',')（新增: $($added -join ', ')）"
}

function Get-LocalIPv4 {
    $ips = @()
    try {
        $ips = @(Get-NetIPAddress -AddressFamily IPv4 -ErrorAction Stop |
            Where-Object { $_.IPAddress -ne '127.0.0.1' -and $_.IPAddress -notlike '169.254.*' } |
            Select-Object -ExpandProperty IPAddress)
    }
    catch {
        $ips = @([System.Net.Dns]::GetHostAddresses([System.Net.Dns]::GetHostName()) |
            Where-Object { $_.AddressFamily -eq 'InterNetwork' } |
            ForEach-Object { $_.IPAddressToString })
    }
    return @($ips | Sort-Object -Unique)
}

function Enable-WinRMFirewall {
    param([int]$Port)

    # 规则名不带空格，避免 netsh 回退时的引号问题
    $ruleName = "Doodle-WinRM-HTTP-$Port"
    try {
        $existing = Get-NetFirewallRule -DisplayName $ruleName -ErrorAction SilentlyContinue
        if ($existing) {
            Set-NetFirewallRule -DisplayName $ruleName -Enabled True -Action Allow -Profile Any -ErrorAction Stop
            Write-Host "  [OK] 防火墙规则已启用: $ruleName (TCP $Port, 全部配置文件)"
        }
        else {
            New-NetFirewallRule -DisplayName $ruleName -Direction Inbound -Action Allow `
                -Protocol TCP -LocalPort $Port -Profile Any `
                -Description 'Doodle PowerShell 远程访问 (WinRM HTTP)' -ErrorAction Stop | Out-Null
            Write-Host "  [OK] 已创建防火墙规则: $ruleName (TCP $Port, 全部配置文件)"
        }

        # 顺带打开系统自带的 WinRM HTTP-In 入站规则
        Get-NetFirewallRule -DisplayGroup 'Windows Remote Management' -ErrorAction SilentlyContinue |
            Where-Object { $_.Direction -eq 'Inbound' -and $_.DisplayName -like '*HTTP-In*' -and $_.Enabled -eq 'False' } |
            ForEach-Object {
                Enable-NetFirewallRule -Name $_.Name -ErrorAction SilentlyContinue
                Write-Host "  [OK] 已启用系统规则: $($_.DisplayName)"
            }
    }
    catch {
        Write-Host "  [警告] NetFirewall 命令不可用（$($_.Exception.Message)），回退到 netsh" -ForegroundColor Yellow
        & netsh advfirewall firewall add rule "name=$ruleName" dir=in action=allow protocol=TCP localport=$Port profile=any 2>&1 | Out-Null
        Write-Host "  [OK] netsh 已添加规则: $ruleName (TCP $Port)"
    }
}

# ---------------------------------------------------------------- 主流程

Write-Host ''
Write-Host '==== Doodle PowerShell 远程访问配置 ====' -ForegroundColor Cyan
$PolicyText = if ($SkipExecutionPolicy) { '跳过' } else { $ExecutionPolicy }
Write-Host "模式: $Mode    端口: $Port    执行策略: $PolicyText"
Write-Host ''

# 1. 管理员权限（Enable-PSRemoting / LocalMachine 执行策略 / 防火墙都需要）
if (-not (Test-IsAdmin)) {
    if ($NoElevate) {
        throw '需要管理员权限：请用「以管理员身份运行」的 PowerShell 重新执行本脚本（或去掉 -NoElevate 让其自动提权）。'
    }
    Write-Host '[提权] 当前不是管理员，正在以管理员身份重新启动...' -ForegroundColor Yellow
    $hostExe = (Get-Process -Id $PID).Path
    if (-not $hostExe) { $hostExe = 'powershell.exe' }

    $forward = @()
    foreach ($key in $PSBoundParameters.Keys) {
        if ($key -eq 'NoElevate') { continue }
        $value = $PSBoundParameters[$key]
        if ($value -is [switch]) {
            if ($value.IsPresent) { $forward += "-$key" }
        }
        elseif ($value -is [array]) {
            $forward += "-$key"
            $forward += (($value | ForEach-Object { "'$_'" }) -join ',')
        }
        else {
            $forward += "-$key"
            $forward += "'$value'"
        }
    }
    $argList = @('-NoProfile', '-ExecutionPolicy', 'Bypass', '-File', "`"$PSCommandPath`"", '-NoElevate') + $forward
    $child = Start-Process -FilePath $hostExe -ArgumentList $argList -Verb RunAs -PassThru -Wait
    exit $child.ExitCode
}
Write-Host '[权限] 已获得管理员权限'

# 2. 执行策略（自身进程 + 两个 PowerShell 宿主）
if (-not $SkipExecutionPolicy) {
    Write-Host '[执行策略] 调整本机 PowerShell 脚本执行策略'
    try {
        Set-ExecutionPolicy -ExecutionPolicy Bypass -Scope Process -Force -ErrorAction Stop
        Write-Host '  [OK] Process = Bypass（仅当前进程，不改动系统设置）'
    }
    catch {
        Write-Host "  [跳过] Process: $($_.Exception.Message)" -ForegroundColor Yellow
    }

    foreach ($scope in @('CurrentUser', 'LocalMachine')) {
        try {
            Set-ExecutionPolicy -ExecutionPolicy $ExecutionPolicy -Scope $scope -Force -ErrorAction Stop
            Write-Host "  [OK] $scope = $ExecutionPolicy"
        }
        catch {
            Write-Host "  [失败] ${scope}: $($_.Exception.Message)" -ForegroundColor Yellow
        }
    }

    Set-HostExecutionPolicy -HostExe 'powershell.exe' -Policy $ExecutionPolicy
    Set-HostExecutionPolicy -HostExe 'pwsh.exe' -Policy $ExecutionPolicy

    $machinePolicy = Get-ExecutionPolicy -Scope MachinePolicy
    if ($machinePolicy -ne 'Undefined') {
        Write-Warning "组策略 MachinePolicy = $machinePolicy，优先级高于本脚本设置，如需修改请联系域管理员。"
    }
}

# 3. 服务端：让别的机器可以通过 IP 连进来
if ($Mode -eq 'Both' -or $Mode -eq 'Server') {
    Write-Host '[服务端] 允许其他机器通过 IP 连接本机'

    if (-not (Get-Service -Name WinRM -ErrorAction SilentlyContinue)) {
        throw '本机未安装 WinRM 服务，无法开启 PowerShell 远程访问。'
    }
    Set-Service -Name WinRM -StartupType Automatic
    if ((Get-Service -Name WinRM).Status -ne 'Running') {
        Start-Service -Name WinRM
    }
    Write-Host "  [OK] WinRM 服务: $((Get-Service -Name WinRM).Status) / 启动类型 Automatic"

    # 监听器：Address=* 表示监听本机全部 IP
    $enableArgs = @{ Force = $true }
    if ($AllowAnyNetworkProfile) { $enableArgs['SkipNetworkProfileCheck'] = $true }
    try {
        Enable-PSRemoting @enableArgs -ErrorAction Stop | Out-Null
        Write-Host '  [OK] Enable-PSRemoting 完成'
    }
    catch {
        Write-Host "  [警告] Enable-PSRemoting 失败: $($_.Exception.Message)" -ForegroundColor Yellow
        Write-Host '         尝试手动创建监听器...'
        if (-not (Get-ChildItem -Path 'WSMan:\localhost\Listener' -ErrorAction SilentlyContinue)) {
            New-Item -Path 'WSMan:\localhost\Listener' -Transport HTTP -Address '*' -Force | Out-Null
        }
    }

    $listeners = @(Get-ChildItem -Path 'WSMan:\localhost\Listener' -ErrorAction SilentlyContinue)
    if ($listeners.Count -eq 0) {
        throw '未创建任何 WinRM 监听器，请检查 WinRM 服务状态后重试。'
    }
    foreach ($listener in $listeners) {
        $listenerPath = "WSMan:\localhost\Listener\$($listener.Name)"
        $enabled = (Get-Item -Path "$listenerPath\Enabled" -ErrorAction SilentlyContinue).Value
        $transport = (Get-Item -Path "$listenerPath\Transport" -ErrorAction SilentlyContinue).Value
        $address = (Get-Item -Path "$listenerPath\Address" -ErrorAction SilentlyContinue).Value
        Write-Host "  [OK] 监听器 $($listener.Name): Transport=$transport Address=$address Enabled=$enabled"
        if ($enabled -ne 'true') {
            Set-Item -Path "$listenerPath\Enabled" -Value $true -Force
            Write-Host '       -> 已置为启用'
        }
    }

    # 用 IP 访问必须监听全部地址
    Set-Item -Path 'WSMan:\localhost\Service\IPv4Filter' -Value '*' -Force
    Set-Item -Path 'WSMan:\localhost\Service\IPv6Filter' -Value '*' -Force
    Write-Host '  [OK] Service IPv4Filter/IPv6Filter = *（允许通过任意 IP 访问）'

    # Basic 认证 + 允许未加密，配合 -Authentication Basic 使用
    Set-Item -Path 'WSMan:\localhost\Service\Auth\Basic' -Value $true -Force
    Set-Item -Path 'WSMan:\localhost\Service\AllowUnencrypted' -Value $true -Force
    Write-Host '  [OK] Service\Auth\Basic = true, Service\AllowUnencrypted = true'

    if ($EnableCredSSP) {
        Set-Item -Path 'WSMan:\localhost\Service\Auth\CredSSP' -Value $true -Force
        Write-Host '  [OK] Service\Auth\CredSSP = true（支持远程会话二次跳转）'
    }

    Enable-WinRMFirewall -Port $Port
}

# 4. 客户端：让本机可以连别的机器
if ($Mode -eq 'Both' -or $Mode -eq 'Client') {
    Write-Host '[客户端] 允许本机通过 IP 连接其他机器'
    Add-TrustedHost -Hosts $TrustedHosts
    Set-Item -Path 'WSMan:\localhost\Client\Auth\Basic' -Value $true -Force
    Set-Item -Path 'WSMan:\localhost\Client\AllowUnencrypted' -Value $true -Force
    Write-Host '  [OK] Client\Auth\Basic = true, Client\AllowUnencrypted = true'

    if ($EnableCredSSP) {
        Set-Item -Path 'WSMan:\localhost\Client\Auth\CredSSP' -Value $true -Force
        Write-Host '  [OK] Client\Auth\CredSSP = true'
    }
}

# 5. 自检
$localIps = Get-LocalIPv4
if (-not $SkipVerify) {
    Write-Host '[自检]'
    Write-Host '  当前监听器:'
    (winrm enumerate winrm/config/listener 2>$null) | ForEach-Object { Write-Host "    $_" }

    foreach ($ip in $localIps) {
        try {
            $null = Test-WSMan -ComputerName $ip -ErrorAction Stop
            Write-Host "  [OK] Test-WSMan $ip 成功" -ForegroundColor Green
        }
        catch {
            Write-Host "  [注意] Test-WSMan $ip 失败: $($_.Exception.Message)" -ForegroundColor Yellow
            Write-Host "         若 TrustedHosts 未包含 $ip，请在客户端侧执行 -Mode Client -TrustedHosts '$ip'"
        }
    }

    try {
        $ports = @(Get-NetTCPConnection -LocalPort $Port -State Listen -ErrorAction SilentlyContinue |
            Select-Object -ExpandProperty LocalAddress)
        if ($ports.Count -gt 0) {
            Write-Host "  [OK] TCP $Port 正在监听: $($ports -join ', ')" -ForegroundColor Green
        }
        else {
            Write-Host "  [注意] TCP $Port 未处于监听状态" -ForegroundColor Yellow
        }
    }
    catch {
        Write-Host "  [跳过] 端口检查: $($_.Exception.Message)" -ForegroundColor Yellow
    }
}

# 6. 结果与后续用法
Write-Host ''
Write-Host '==== 配置完成 ====' -ForegroundColor Cyan
Write-Host "本机 IPv4: $(if ($localIps.Count -gt 0) { $localIps -join ', ' } else { '未检测到' })"
Write-Host "本机名称 : $env:COMPUTERNAME"
Write-Host ''
Write-Host '在其他机器上连接本机（对方也需要配置客户端侧，可用本脚本 -Mode Client）：' -ForegroundColor Gray
Write-Host "  `$cred = Get-Credential administrator"
Write-Host "  New-PSSession -ComputerName <本机IP> -Credential `$cred -Authentication Basic"
Write-Host ''
Write-Host '安全提示：Basic + AllowUnencrypted 为明文传输，仅限可信内网；' -ForegroundColor Yellow
Write-Host '         建议把 -TrustedHosts 收窄为具体 IP，生产环境改用 HTTPS(5986)。' -ForegroundColor Yellow
