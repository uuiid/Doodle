class OffDays {
    [string]$name
    [string]$date
    [bool]$isOffDay
}
#Set-Item WSMan:\localhost\Client\TrustedHosts -Value '*'
#Set-Item WSMan:\localhost\Client\Auth\Basic -Value True
#Set-Item WSMan:\localhost\Service\Auth\Basic -Value True
#Set-Item WSMan:\localhost\Client\AllowUnencrypted -Value True
#Set-Item WSMan:\localhost\Service\AllowUnencrypted -Value True
# Get-ExecutionPolicy
# Set-ExecutionPolicy RemoteSigned
function Add-Compensatory() {
    param (
        [string] $Path,
        [System.Management.Automation.PSObject[]]$OffDays
    )
    if (-not (Test-Path -Path $Path)) {
        Write-Host "文件不存在 $Path"
        return
    }
    $Json = Get-Content -Path $Path -Raw -Encoding UTF8 | ConvertFrom-Json;
    foreach ($day in $OffDays) {
        $it = $Json.days | Where-Object { $_.date -eq $day.date }
        if ($it) {
            $it.isOffDay = $day.isOffDay
            $it.name = $day.name
        }
        else {
            $Json.days += $day
        }
    }
    $Json | ConvertTo-Json  | Set-Content -Path $Path -Encoding UTF8
}
function Compress-UEPlugins() {
    param(
        [string]$UEVersion,
        [string]$DoodleVersion,
        [string]$DoodleGitRoot,
        [string]$OutPath
    )

    if (-Not (Test-Path "$OutPath\dist\Plugins")) {
        New-Item "$OutPath\dist\Plugins" -ItemType Directory
    }
    $UEPluginsPath = "$OutPath\dist\Plugins\Doodle_$DoodleVersion.$UEVersion.zip"
    if (Test-Path $UEPluginsPath) {
        Write-Host "UE插件包已存在: $UEPluginsPath"
        return
    }
    $UEPluginsJsonPath = Convert-Path "$DoodleGitRoot/script/uePlug/$UEVersion/Plugins/Doodle/Doodle.uplugin"
    $UEPluginsJson = Get-Content -Path $UEPluginsJsonPath -Raw -Encoding UTF8 | ConvertFrom-Json
    $UEPluginsJson.VersionName = $DoodleVersion
    $UEPluginsJson.Version = [int]($DoodleVersion -replace "\.", "")
    # 判断属性 EnabledByDefault 是否存在
    if (-not $UEPluginsJson.PSObject.Properties["EnabledByDefault"]) {
        $UEPluginsJson | Add-Member -MemberType NoteProperty -Name "EnabledByDefault" -Value $true
    }
    else {
        $UEPluginsJson.EnabledByDefault = $true
    }

    $UEPluginsJson | ConvertTo-Json | Set-Content -Path $UEPluginsJsonPath -Encoding UTF8

    Compress-Archive -Path "$DoodleGitRoot\script\uePlug\$UEVersion\Plugins\Doodle" -DestinationPath "$OutPath\dist\Plugins\Doodle_$DoodleVersion.$UEVersion.zip"
}

function Get-GitKitsuCommendID() {
    $DoodleKitsuRoot = "E:\source\kitsu"
    $TempPath = "$env:TEMP/doodle_kitsu_git.txt"
    Start-Process -FilePath "git.exe" -ArgumentList  "rev-parse", "HEAD" -WorkingDirectory $DoodleKitsuRoot -NoNewWindow -Wait -RedirectStandardOutput $TempPath

    $id = Get-Content $TempPath
    return $id;
}

function Initialize-Doodle {
    param(
        [string]$OutPath,
        [switch]$OnlyOne
    )
    if (-not (Test-Path $OutPath)) {
        New-Item $OutPath -ItemType Directory
    }
    $DoodleTMPDir = [System.IO.Path]::GetTempPath()
    $DoodleGitRoot = Convert-Path "$PSScriptRoot/../"
    $DoodleBuildRoot = Convert-Path "$DoodleGitRoot/build"
    $DoodleBuildRelease = Convert-Path "$DoodleBuildRoot/Ninja_release"
    $timestamp = Get-Date -Format o | ForEach-Object { $_ -replace ":", "." }
    $DoodleLogPath = $DoodleTMPDir + "\build_$timestamp.log"
    $Tags = git tag --sort=-v:refname;
    # 去除 前缀 v
    $Tags = $Tags | ForEach-Object { $_ -replace "v", "" }
    $DoodleVersion = $Tags[0]
    Write-Host "当前最新版本号: $DoodleVersion"
    $DoodleSource = Convert-Path "$DoodleBuildRoot/Ninja_release/_CPack_Packages/win64/ZIP/Doodle-$DoodleVersion-win64"
    $DoodleKitsuRoot = "E:\source\kitsu"
    $DoodleTimePath = "$DoodleBuildRoot\holiday-cn"
    $DoodleExePath = "E:\source\doodle\dist\索以魔盒.exe"
    Write-Host "开始检查文件"
    $id = Get-GitKitsuCommendID

    $NpmResult = Start-Process -FilePath "git.exe" -ArgumentList  "pull", "loc", "master_sy_new3" -WorkingDirectory $DoodleKitsuRoot -NoNewWindow -Wait -PassThru
    if ($NpmResult.ExitCode -ne 0) {
        # 抛出异常
        throw "拉取失败"
    }
    if ($id -ne (Get-GitKitsuCommendID)) {
        Write-Host "开始构建 Kitsu"
        $NpmResult = Start-Process -FilePath "powershell.exe" -ArgumentList  "$Env:APPDATA/npm/npm.ps1", "run", "build" -WorkingDirectory $DoodleKitsuRoot -RedirectStandardOutput $DoodleLogPath -NoNewWindow -Wait -PassThru
        if ($NpmResult.ExitCode -ne 0) {
            # 抛出异常
            throw "构建失败"
        }
    }
    Write-Host "开始备份pdb文件"
    if (-not (Test-Path "$DoodleBuildRoot\pdb\$DoodleVersion\")) {
        New-Item -Path "$DoodleBuildRoot\pdb\$DoodleVersion\" -ItemType Directory
    }
    Copy-Item -Path "$DoodleBuildRoot\Ninja_release\bin\*.pdb" -Destination "$DoodleBuildRoot\pdb\$DoodleVersion\" -Force

    Write-Host "开始复制文件"
    Write-Host "robocopy 日志 $DoodleLogPath"
    &robocopy "$DoodleSource\bin" "$OutPath\bin" /MIR /unilog+:$DoodleLogPath | Out-Null
    &robocopy "$DoodleKitsuRoot\dist" "$OutPath\dist" /MIR /unilog+:$DoodleLogPath /xd "video" "Plugins" "time" /xf "*.zip" | Out-Null
    # 复制安装包
    if ( -not $OnlyOne) {
        &Robocopy "$DoodleBuildRoot\video" "$OutPath\dist\video" /MIR /unilog+:$DoodleLogPath | Out-Null
        # 添加UE插件安装
        Compress-UEPlugins -UEVersion "5.5" -DoodleVersion $DoodleVersion -DoodleGitRoot $DoodleGitRoot -OutPath $OutPath
        Compress-Archive -Path $DoodleGitRoot\script\uePlug\SideFX_Labs -DestinationPath $OutPath\dist\Plugins\SideFX_Labs.zip -Force
        &Robocopy "$DoodleBuildRelease\" "$OutPath\dist" "*.zip" /unilog+:$DoodleLogPath | Out-Null
        $Tags = $Tags[0..100]
        [array]::Reverse($Tags)
        #         寻找版本号 3.6.678 并放在最后
        #        $DoodleVersionList = $DoodleVersionList | Where-Object { $_ -ne "3.6.678" }
        #        $DoodleVersionList += "`n3.6.678"
    }
    Set-Content -Path "$OutPath\dist\version.txt" -Value ($Tags -join "`n") -NoNewline

    Copy-Item $DoodleExePath -Destination "$OutPath\dist"

    # 从github 下载网络资源
    # 先查看标签, 再组合下载url

    # 检查文件是否是三个月以上的旧文件
    if ((Get-ChildItem "$DoodleBuildRoot/holiday-cn-*.zip")[-1].LastWriteTime - (Get-Date).AddMonths(-3) -lt 0) {
        $tag = (Invoke-WebRequest -Uri https://api.github.com/repos/NateScarlet/holiday-cn/tags -Headers @{ "accept" = "application/json" } |
            ConvertFrom-Json)[0].name
        # 检查文件是否存在
        Invoke-WebRequest -Uri https://github.com/NateScarlet/holiday-cn/releases/latest/download/holiday-cn-$tag.zip -OutFile "$DoodleBuildRoot\holiday-cn-$tag.zip"
        Expand-Archive -Path "$DoodleBuildRoot\holiday-cn-$tag.zip" -DestinationPath $DoodleTimePath -Force
    }
    else {
        Expand-Archive -Path (Get-ChildItem "$DoodleBuildRoot/holiday-cn-*.zip")[-1].FullName -DestinationPath $DoodleTimePath -Force
    }
    Add-Compensatory -Path "$DoodleTimePath\2025.json" @(
        [System.Management.Automation.PSObject]@{
            name     = "公司年假补班"
            date     = "2025-01-11"
            isOffDay = $false
        },
        [System.Management.Automation.PSObject]@{
            name     = "公司放假"
            date     = "2025-01-26"
            isOffDay = $true
        },
        [System.Management.Automation.PSObject]@{
            name     = "公司放假"
            date     = "2025-01-27"
            isOffDay = $true
        },
        [System.Management.Automation.PSObject]@{
            name     = "公司放假"
            date     = "2025-02-05"
            isOffDay = $true
        },
        [System.Management.Automation.PSObject]@{
            name     = "公司年假补班"
            date     = "2025-12-28"
            isOffDay = $false
        }
    )
    Add-Compensatory -Path "$DoodleTimePath\2026.json" @(
        [System.Management.Automation.PSObject]@{
            name     = "公司年假补班"
            date     = "2026-01-24"
            isOffDay = $false
        }, 
        [System.Management.Automation.PSObject]@{
            name     = "公司年假补班"
            date     = "2026-02-07"
            isOffDay = $false
        }
    )
    &robocopy $DoodleTimePath "$OutPath\dist\time" /MIR /unilog+:$DoodleLogPath | Out-Null
    &robocopy $DoodleTimePath "$DoodleKitsuRoot\dist\time" /MIR /unilog+:$DoodleLogPath | Out-Null

    return $DoodleVersion;
}


function New-ServerPSSession {
    $RootPassword = ConvertTo-SecureString "root" -AsPlainText -Force
    $Credential = New-Object -TypeName System.Management.Automation.PSCredential -ArgumentList administrator, $RootPassword
    $NewSession = Get-PSSession -ComputerName 192.168.40.181 -ErrorAction SilentlyContinue
    if (-not $NewSession) {
        $NewSession = New-PSSession -ComputerName 192.168.40.181 -Credential $Credential -Authentication Basic
    }
    return $NewSession;
}

Export-ModuleMember -Function Initialize-Doodle , New-ServerPSSession
