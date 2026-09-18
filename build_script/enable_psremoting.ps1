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

    执行策略部分不会中断脚本：任何失败都会打印诊断并继续配置 WinRM。

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
    踩坑记录（已在真实机器上复现并修掉，勿回退）：

    1) Set-ExecutionPolicy 报「安全性错误。/ Security error / Sicherheitsfehler」时，
       CategoryInfo 是 PermissionDenied、FullyQualifiedErrorId 是 ExecutionPolicyOverride。
       含义是「要写的这个 scope 被更高优先级的 scope 覆盖」——不是权限不足。
       优先级：MachinePolicy > UserPolicy > Process > CurrentUser > LocalMachine。
       最常见的原因不是组策略，而是 Process scope 被占用：进程只要是用
       -ExecutionPolicy Bypass 启动的（或脚本自己设了 Process = Bypass），
       再写 CurrentUser / LocalMachine 就必然报这个错（本机实测：MachinePolicy 与
       UserPolicy 都是 Undefined 时同样报错）。
       修法：先 Set-ExecutionPolicy -Scope Process -ExecutionPolicy Undefined 清掉
       Process，再写低优先级 scope；本脚本已自动这么做。
       是否真被组策略锁定，用 Get-ExecutionPolicy -List 看 MachinePolicy / UserPolicy 两行。

    2) Windows PowerShell 5.1 作为调用方时，不会转义传给原生命令的参数里的双引号：
       5.1 调 powershell.exe -Command '<含双引号的脚本>' 时双引号会被整个吃掉，
       子进程报 "Missing argument in parameter list"（line:1 char:29）。
       所以跨宿主设置执行策略一律用 -EncodedCommand 传 base64，不要用 -Command。
       同时不要给该子进程加 -ExecutionPolicy Bypass，否则它自己的 Process scope
       又会触发上面第 1 条的覆盖问题。

    3) $ErrorActionPreference = 'Stop' 时，原生命令往 stderr 写内容会被包装成
       终止错误 NativeCommandFailed，直接把整个脚本打断（表现为脚本中途静默结束，
       后面的 WinRM 配置根本没跑）。所有原生命令调用统一走 Invoke-NativeSafe
       （内部临时把 EAP 降为 Continue 并吞掉 stderr）。

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
$script:Warnings = @()

function Add-Warning {
    param([string]$Text)
    $script:Warnings += $Text
}

# ---------------------------------------------------------------- 工具函数

function Test-IsAdmin {
    $identity = [Security.Principal.WindowsIdentity]::GetCurrent()
    $principal = New-Object Security.Principal.WindowsPrincipal($identity)
    return $principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
}

# 当前跑在哪个宿主上：5.1 与 7 的执行策略是分开存的
function Get-CurrentHostExeName {
    if ($PSVersionTable.PSEdition -eq 'Core') { return 'pwsh.exe' }
    return 'powershell.exe'
}

<#
    原生命令安全调用：
    $ErrorActionPreference = 'Stop' 时，原生命令写 stderr 会产生终止错误 NativeCommandFailed，
    直接把脚本打断（实测：powershell.exe 报解析错误 → 整个脚本在此行终止）。
    这里临时降为 Continue 并把 stdout/stderr 一起收集成字符串，绝不抛异常、绝不外泄。
#>
function Invoke-NativeSafe {
    param(
        [string]$FilePath,
        [string[]]$Arguments
    )
    $oldPreference = $ErrorActionPreference
    $ErrorActionPreference = 'Continue'
    try {
        $output = & $FilePath @Arguments 2>&1
        return @($output | ForEach-Object { "$_" })
    }
    catch {
        return @("[执行异常] $($_.Exception.Message)")
    }
    finally {
        $ErrorActionPreference = $oldPreference
    }
}

# 5.1 与 7 的执行策略分开存储，WinRM 远程会话默认跑 5.1，所以两个宿主都要设置。
# 当前宿主在上面的 scope 循环里已经设过，这里只负责「另一个」宿主。
function Set-HostExecutionPolicy {
    param(
        [string]$HostExe,
        [string]$Policy
    )

    if ($HostExe -eq (Get-CurrentHostExeName)) {
        Write-Host "  [跳过] ${HostExe} 是当前宿主，已由上面的 scope 循环处理"
        return
    }

    $command = Get-Command $HostExe -ErrorAction SilentlyContinue
    if (-not $command) {
        Write-Host "  [跳过] 未找到 $HostExe（该版本 PowerShell 未安装）"
        return
    }

    # 子进程里必须先清掉 Process scope：只要子进程带着 -ExecutionPolicy 启动，
    # Process = Bypass 就会把 CurrentUser/LocalMachine 的写入判为「被覆盖」而抛
    # ExecutionPolicyOverride（消息即「安全性错误」）。
    $inner = 'Set-ExecutionPolicy -Scope Process -ExecutionPolicy Undefined -Force -ErrorAction SilentlyContinue; ' +
    'foreach ($s in @("CurrentUser","LocalMachine")) { try { Set-ExecutionPolicy -ExecutionPolicy ' + $Policy +
    ' -Scope $s -Force -ErrorAction Stop; "OK   " + $s + " = ' + $Policy + '" } catch { "FAIL " + $s + " : " + $_.FullyQualifiedErrorId + " / " + $_.Exception.Message } }'

    # 必须用 -EncodedCommand：Windows PowerShell 5.1 当调用方时不会转义参数里的双引号，
    # 用 -Command 会让子进程收到被吞掉双引号的残缺脚本（Missing argument in parameter list）。
    # 也刻意不给子进程加 -ExecutionPolicy Bypass：-EncodedCommand 不受执行策略限制，
    # 加了反而会让子进程 Process = Bypass，重新触发上面的覆盖问题。
    $encoded = [Convert]::ToBase64String([System.Text.Encoding]::Unicode.GetBytes($inner))
    $lines = Invoke-NativeSafe -FilePath $command.Source `
        -Arguments @('-NoProfile', '-NonInteractive', '-EncodedCommand', $encoded)

    foreach ($line in $lines) {
        Write-Host "    ${HostExe} -> $line"
        if ($line -like 'FAIL*') { Add-Warning "$HostExe 执行策略设置失败: $line" }
    }
}

# 执行策略诊断：区分「Process scope 覆盖」（本脚本可自动修）和「组策略锁定」（脚本无能为力）
function Show-ExecutionPolicyDiagnosis {
    param([switch]$OverrideFailed)

    $list = @(Get-ExecutionPolicy -List)
    Write-Host '  [诊断] 当前执行策略：'
    foreach ($item in $list) {
        Write-Host ("         {0,-14} {1}" -f $item.Scope, $item.ExecutionPolicy)
    }

    $gpoScopes = @($list | Where-Object { $_.Scope -eq 'MachinePolicy' -or $_.Scope -eq 'UserPolicy' } |
        Where-Object { $_.ExecutionPolicy -and $_.ExecutionPolicy -ne 'Undefined' })
    if ($gpoScopes.Count -eq 0) {
        if ($OverrideFailed) {
            Write-Host '         MachinePolicy / UserPolicy 均为 Undefined，不是组策略锁定；' -ForegroundColor Yellow
            Write-Host '         ExecutionPolicyOverride 来自 Process scope 覆盖，本脚本已自动清除 Process。' -ForegroundColor Yellow
        }
        return $false
    }

    Write-Host '  [诊断] 执行策略被组策略锁定，脚本无法修改（FullyQualifiedErrorId = ExecutionPolicyOverride）：' -ForegroundColor Yellow
    foreach ($scope in $gpoScopes) {
        Write-Host "         $($scope.Scope) = $($scope.ExecutionPolicy)" -ForegroundColor Yellow
    }
    foreach ($key in @('HKLM:\SOFTWARE\Policies\Microsoft\Windows\PowerShell',
            'HKCU:\SOFTWARE\Policies\Microsoft\Windows\PowerShell',
            'HKLM:\SOFTWARE\Policies\Microsoft\PowerShellCore')) {
        if (Test-Path $key) {
            $value = Get-ItemProperty -Path $key -ErrorAction SilentlyContinue
            Write-Host "         $key : ExecutionPolicy=$($value.ExecutionPolicy) EnableScripts=$($value.EnableScripts)" -ForegroundColor Yellow
        }
    }
    Write-Host '         → 修改位置：gpedit.msc → 计算机配置 → 管理模板 → Windows 组件 → Windows PowerShell → 打开脚本执行；' -ForegroundColor Yellow
    Write-Host '           域环境需在域控上改 GPO（组策略优先级高于本脚本的任何 scope）。' -ForegroundColor Yellow
    Write-Host '         → 本次远程配置不受影响（本脚本是以 -ExecutionPolicy Bypass 启动的）；' -ForegroundColor Yellow
    Write-Host '           远程会话里执行内联脚本块（Invoke-Command -ScriptBlock）也不受执行策略限制，' -ForegroundColor Yellow
    Write-Host '           只有运行 .ps1 文件才会被挡。' -ForegroundColor Yellow
    return $true
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
        Add-Warning "NetFirewall 不可用，已回退 netsh"
        $result = Invoke-NativeSafe -FilePath 'netsh' `
            -Arguments @('advfirewall', 'firewall', 'add', 'rule', "name=$ruleName", 'dir=in', 'action=allow', 'protocol=TCP', "localport=$Port", 'profile=any')
        Write-Host "  [OK] netsh 已添加规则: $ruleName (TCP $Port)"
        foreach ($line in $result) { if ($line.Trim()) { Write-Host "        $line" } }
    }
}

# ---------------------------------------------------------------- 主流程

Write-Host ''
Write-Host '==== Doodle PowerShell 远程访问配置 ====' -ForegroundColor Cyan
$PolicyText = if ($SkipExecutionPolicy) { '跳过' } else { $ExecutionPolicy }
Write-Host "模式: $Mode    端口: $Port    执行策略: $PolicyText"
Write-Host "宿主: $(Get-CurrentHostExeName) $($PSVersionTable.PSVersion)"
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

# 2. 执行策略（自身进程 + 两个 PowerShell 宿主）；本步骤任何失败都不中断后续配置
if (-not $SkipExecutionPolicy) {
    $overrideFailed = $false
    try {
        Write-Host '[执行策略] 调整本机 PowerShell 脚本执行策略'

        # Process scope 的优先级高于 CurrentUser / LocalMachine。
        # 用 -ExecutionPolicy Bypass 启动本脚本时，Process 已经是 Bypass，此时再写低优先级 scope
        # 会抛 ExecutionPolicyOverride（CategoryInfo = PermissionDenied，消息就是「安全性错误。」）。
        # 先清掉 Process，低优先级 scope 才写得进、也才真正生效。
        $processPolicy = Get-ExecutionPolicy -Scope Process
        if ($processPolicy -and $processPolicy -ne 'Undefined') {
            try {
                Set-ExecutionPolicy -ExecutionPolicy Undefined -Scope Process -Force -ErrorAction Stop
                Write-Host "  [OK] 已清除 Process = $processPolicy（它优先级更高，会挡住下面两个 scope）"
            }
            catch {
                Write-Host "  [注意] 清除 Process scope 失败: $($_.Exception.Message)" -ForegroundColor Yellow
            }
        }

        foreach ($scope in @('CurrentUser', 'LocalMachine')) {
            try {
                Set-ExecutionPolicy -ExecutionPolicy $ExecutionPolicy -Scope $scope -Force -ErrorAction Stop
                Write-Host "  [OK] ${scope} = $ExecutionPolicy"
            }
            catch {
                $errorId = "$($_.FullyQualifiedErrorId)"
                Write-Host "  [注意] ${scope}: $($_.Exception.Message)" -ForegroundColor Yellow
                Write-Host "         FullyQualifiedErrorId = $errorId" -ForegroundColor Yellow
                if ($errorId -like 'ExecutionPolicyOverride*') { $overrideFailed = $true }
                Add-Warning "执行策略 ${scope} 设置失败: $errorId"
            }
        }

        Set-HostExecutionPolicy -HostExe 'powershell.exe' -Policy $ExecutionPolicy
        Set-HostExecutionPolicy -HostExe 'pwsh.exe' -Policy $ExecutionPolicy

        $null = Show-ExecutionPolicyDiagnosis -OverrideFailed:$overrideFailed
    }
    catch {
        Write-Host "  [错误] 执行策略步骤异常，已跳过: $($_.Exception.Message)" -ForegroundColor Red
        Add-Warning "执行策略步骤异常: $($_.Exception.Message)"
    }
}

# 3. 服务端：让别的机器可以通过 IP 连进来
if ($Mode -eq 'Both' -or $Mode -eq 'Server') {
    try {
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
            Add-Warning "Enable-PSRemoting 失败: $($_.Exception.Message)"
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
    catch {
        Write-Host "  [错误] 服务端配置失败: $($_.Exception.Message)" -ForegroundColor Red
        Add-Warning "服务端配置失败: $($_.Exception.Message)"
    }
}

# 4. 客户端：让本机可以连别的机器
if ($Mode -eq 'Both' -or $Mode -eq 'Client') {
    try {
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
    catch {
        Write-Host "  [错误] 客户端配置失败: $($_.Exception.Message)" -ForegroundColor Red
        Add-Warning "客户端配置失败: $($_.Exception.Message)"
    }
}

# 5. 自检
$localIps = Get-LocalIPv4
if (-not $SkipVerify) {
    Write-Host '[自检]'
    Write-Host '  当前监听器:'
    foreach ($line in (Invoke-NativeSafe -FilePath 'winrm' -Arguments @('enumerate', 'winrm/config/listener'))) {
        if ($line.Trim()) { Write-Host "    $line" }
    }

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
if ($script:Warnings.Count -gt 0) {
    Write-Host ''
    Write-Host '需要留意的事项：' -ForegroundColor Yellow
    foreach ($warning in $script:Warnings) { Write-Host "  - $warning" -ForegroundColor Yellow }
}
Write-Host ''
Write-Host '在其他机器上连接本机（对方也需要配置客户端侧，可用本脚本 -Mode Client）：' -ForegroundColor Gray
Write-Host "  `$cred = Get-Credential administrator"
Write-Host "  New-PSSession -ComputerName <本机IP> -Credential `$cred -Authentication Basic"
Write-Host ''
Write-Host '提示：从网络共享（\\...）直接运行本脚本时，建议先复制到本机再执行，' -ForegroundColor Gray
Write-Host '      避免 UNC 路径带来的信任区域/执行策略问题。' -ForegroundColor Gray
Write-Host ''
Write-Host '安全提示：Basic + AllowUnencrypted 为明文传输，仅限可信内网；' -ForegroundColor Yellow
Write-Host '         建议把 -TrustedHosts 收窄为具体 IP，生产环境改用 HTTPS(5986)。' -ForegroundColor Yellow
