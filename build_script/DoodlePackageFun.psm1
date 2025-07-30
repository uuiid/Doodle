class OffDays
{
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
function Add-Compensatory()
{
    param (
        [string] $Path,
        [System.Management.Automation.PSObject[]]$OffDays
    )
    if (-not (Test-Path -Path $Path))
    {
        Write-Host "文件不存在 $Path"
        return
    }
    $Json = Get-Content -Path $Path -Raw -Encoding UTF8 | ConvertFrom-Json;
    foreach ($day in $OffDays)
    {
        $it = $Json.days | Where-Object { $_.date -eq $day.date }
        if ($it)
        {
            $it.isOffDay = $day.isOffDay
            $it.name = $day.name
        }
        else
        {
            $Json.days += $day
        }
    }
    $Json | ConvertTo-Json  | Set-Content -Path $Path -Encoding UTF8
}
function Compress-UEPlugins()
{
    param(
        [string]$UEVersion,
        [string]$DoodleVersion,
        [string]$DoodleGitRoot,
        [string]$OutPath
    )

    if (-Not (Test-Path "$OutPath\dist\Plugins"))
    {
        New-Item "$OutPath\dist\Plugins" -ItemType Directory
    }

    $UEPluginsJsonPath = Convert-Path "$DoodleGitRoot/script/uePlug/$UEVersion/Plugins/Doodle/Doodle.uplugin"
    $UEPluginsJson = Get-Content -Path $UEPluginsJsonPath -Raw -Encoding UTF8 | ConvertFrom-Json
    $UEPluginsJson.VersionName = $DoodleVersion
    $UEPluginsJson.Version = [int]($DoodleVersion -replace "\.", "")
    # 判断属性 EnabledByDefault 是否存在
    if (-not $UEPluginsJson.PSObject.Properties["EnabledByDefault"])
    {
        $UEPluginsJson | Add-Member -MemberType NoteProperty -Name "EnabledByDefault" -Value $true
    }
    else
    {
        $UEPluginsJson.EnabledByDefault = $true
    }

    $UEPluginsJson | ConvertTo-Json | Set-Content -Path $UEPluginsJsonPath -Encoding UTF8

    Compress-Archive -Path "$DoodleGitRoot\script\uePlug\$UEVersion\Plugins\Doodle" -DestinationPath "$OutPath\dist\Plugins\Doodle_$DoodleVersion.$UEVersion.zip"
}

function Initialize-Doodle
{
    param(
        [string]$OutPath,
        [Switch]$BuildKitsu,
        [switch]$OnlyOne
    )
    if (-not (Test-Path $OutPath))
    {
        New-Item $OutPath -ItemType Directory
    }
    $DoodleGitRoot = Convert-Path "$PSScriptRoot/../"
    $DoodleBuildRoot = Convert-Path "$DoodleGitRoot/build"
    $DoodleBuildRelease = Convert-Path "$DoodleBuildRoot/Ninja_release"
    $timestamp = Get-Date -Format o | ForEach-Object { $_ -replace ":", "." }
    $DoodleLogPath = $DoodleBuildRoot + "\build_$timestamp.log"
    $DoodleVersion = ((Get-ChildItem "$DoodleBuildRoot/Ninja_release/_CPack_Packages/win64/ZIP" -Directory)[0] -split "-")[1]
    $DoodleSource = Convert-Path "$DoodleBuildRoot/Ninja_release/_CPack_Packages/win64/ZIP/Doodle-$DoodleVersion-win64"
    $DoodleKitsuRoot = "E:\source\kitsu"
    $DoodleTimePath = "$DoodleBuildRoot\holiday-cn"
    $DoodleExePath = "E:\source\doodle\dist\doodle.exe"
    if ($BuildKitsu)
    {
        Write-Host "开始构建文件"
        $NpmResult = Start-Process -FilePath "powershell.exe" -ArgumentList  "$Env:APPDATA/npm/npm.ps1","run", "build" -WorkingDirectory $DoodleKitsuRoot -RedirectStandardOutput $DoodleLogPath -NoNewWindow -Wait -PassThru
        if ($NpmResult.ExitCode -ne 0)
        {
            # 抛出异常
            throw "构建失败"
        }
    }


    Write-Host "开始复制文件"
    &robocopy "$DoodleSource\bin" "$OutPath\bin" /MIR /unilog+:$DoodleLogPath
    &robocopy "$DoodleKitsuRoot\dist" "$OutPath\dist" /MIR /unilog+:$DoodleLogPath
    &Robocopy "$DoodleBuildRoot\video" "$OutPath\dist\video" /MIR /unilog+:$DoodleLogPath
    # 添加UE插件安装
    Compress-UEPlugins -UEVersion "5.4" -DoodleVersion $DoodleVersion -DoodleGitRoot $DoodleGitRoot -OutPath $OutPath
    Compress-UEPlugins -UEVersion "5.5" -DoodleVersion $DoodleVersion -DoodleGitRoot $DoodleGitRoot -OutPath $OutPath
    Compress-Archive -Path $DoodleGitRoot\script\uePlug\SideFX_Labs -DestinationPath $OutPath\dist\Plugins\SideFX_Labs.zip
    # 复制安装包
    if ($OnlyOne)
    {
        Copy-Item (Get-ChildItem "$DoodleBuildRelease\*" -Include "*.zip")[-1]  -Destination "$OutPath\dist"
        $DoodleVersionList = Get-ChildItem -Path "$DoodleBuildRelease\*" -Include "*.zip" | ForEach-Object { $_.Name.Split("-")[1] }
        Set-Content -Path "$OutPath\dist\version.txt" -Value $DoodleVersionList[-1] -NoNewline
    }
    else
    {
        &Robocopy "$DoodleBuildRelease\" "$OutPath\dist" "*.zip" /unilog+:$DoodleLogPath
        $DoodleVersionList = Get-ChildItem -Path "$DoodleBuildRelease\*" -Include "*.zip" | ForEach-Object { $_.Name.Split("-")[1] }
        #         寻找版本号 3.6.678 并放在最后
        $DoodleVersionList = $DoodleVersionList | Where-Object { $_ -ne "3.6.678" }
        $DoodleVersionList = $DoodleVersionList -join "`n"
        $DoodleVersionList += "`n3.6.678"
        Set-Content -Path "$OutPath\dist\version.txt" -Value ($DoodleVersionList) -NoNewline
    }

    Copy-Item $DoodleExePath -Destination "$OutPath\dist"

    # 从github 下载网络资源
    # 先查看标签, 再组合下载url

    # 检查文件是否是三个月以上的旧文件
    if ((Get-ChildItem "$DoodleBuildRoot/holiday-cn-*.zip")[-1].LastWriteTime - (Get-Date).AddMonths(-3) -lt 0)
    {
        $tag = (Invoke-WebRequest -Uri https://api.github.com/repos/NateScarlet/holiday-cn/tags -Headers @{ "accept" = "application/json" } |
                ConvertFrom-Json)[0].name
        # 检查文件是否存在
        if (-not (Test-Path "$DoodleBuildRoot\holiday-cn-$tag.zip"))
        {
            Invoke-WebRequest -Uri https://github.com/NateScarlet/holiday-cn/releases/latest/download/holiday-cn-$tag.zip -OutFile "$DoodleBuildRoot\holiday-cn-$tag.zip"
        }
        Expand-Archive -Path "$DoodleBuildRoot\holiday-cn-$tag.zip" -DestinationPath $DoodleTimePath -Force
    }
    else
    {
        Expand-Archive -Path (Get-ChildItem "$DoodleBuildRoot/holiday-cn-*.zip")[-1].FullName -DestinationPath $DoodleTimePath -Force
    }


    Add-Compensatory -Path "$DoodleTimePath\2024.json" @(
        [System.Management.Automation.PSObject]@{
            name = "公司年假补班"
            date = "2024-12-14"
            isOffDay = $false
        },
        [System.Management.Automation.PSObject]@{
            name = "公司年假补班"
            date = "2024-12-29"
            isOffDay = $false
        }
    )

    Add-Compensatory -Path "$DoodleTimePath\2025.json" @(
        [System.Management.Automation.PSObject]@{
            name = "公司年假补班"
            date = "2025-01-11"
            isOffDay = $false
        },
        [System.Management.Automation.PSObject]@{
            name = "公司放假"
            date = "2025-01-26"
            isOffDay = $true
        },
        [System.Management.Automation.PSObject]@{
            name = "公司放假"
            date = "2025-01-27"
            isOffDay = $true
        },
        [System.Management.Automation.PSObject]@{
            name = "公司放假"
            date = "2025-02-05"
            isOffDay = $true
        }
    )

    &robocopy $DoodleTimePath "$OutPath\dist\time" /MIR /unilog+:$DoodleLogPath
    &robocopy $DoodleTimePath "$DoodleKitsuRoot\dist\time" /MIR /unilog+:$DoodleLogPath
}


Export-ModuleMember -Function Initialize-Doodle