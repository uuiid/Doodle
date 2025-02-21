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
    $DoodleBuildRoot = Convert-Path "$PSScriptRoot/../build"
    $DoodleSource = Convert-Path (Get-ChildItem "$DoodleBuildRoot/Ninja_release/_CPack_Packages/win64/ZIP" -Directory)[0]
    $DoodleBuildRelease = Convert-Path "$DoodleBuildRoot/Ninja_release"
    $DoodleKitsuRoot = "E:\source\kitsu"
    $DoodleTimePath = "$DoodleKitsuRoot\dist\time"
    $DoodleExePath = "E:\source\doodle\dist\doodle.exe"

    if ($BuildKitsu)
    {
        Write-Host "开始构建文件"
        $NpmResult = Start-Process -FilePath "npm" -ArgumentList "run build" -WorkingDirectory $DoodleKitsuRoot -NoNewWindow -Wait -PassThru
        if ($NpmResult.ExitCode -ne 0)
        {
            # 抛出异常
            throw "构建失败"
        }
    }

    Write-Host "开始复制文件"
    &robocopy "$DoodleSource\bin" "$OutPath\bin" /MIR /np /njh /njs /ns /nc /ndl /fp /ts
    &robocopy "$DoodleKitsuRoot\dist" "$OutPath\dist" /MIR /np /njh /njs /ns /nc /ndl /fp /ts
    # 复制安装包
    if ($OnlyOne)
    {
        Get-ChildItem "$DoodleBuildRelease\*" -Include "*.zip" | Copy-Item -Destination "$OutPath\dist"
        $DoodleVersionList = Get-ChildItem -Path "$DoodleBuildRelease\*" -Include "*.zip" | ForEach-Object { $_.Name.Split("-")[1] }
        Set-Content -Path "$OutPath\dist\version.txt" -Value ($DoodleVersionList -join "`n") -NoNewline
    }
    else
    {
        Copy-Item (Get-ChildItem "$DoodleBuildRelease\*" -Include "*.zip")[-1]  -Destination "$OutPath\dist"
        $DoodleVersionList = Get-ChildItem -Path "$DoodleBuildRelease\*" -Include "*.zip" | ForEach-Object { $_.Name.Split("-")[1] }
        Set-Content -Path "$OutPath\dist\version.txt" -Value $DoodleVersionList[-1] -NoNewline
    }

    Copy-Item $DoodleExePath -Destination "$OutPath\dist"

    # 从github 下载网络资源
    # 先查看标签, 再组合下载url
    $tag = (Invoke-WebRequest -Uri https://api.github.com/repos/NateScarlet/holiday-cn/tags -Headers @{ "accept" = "application/json" } |
            ConvertFrom-Json)[0].name
    # 检查文件是否存在
    if (-not (Test-Path "$DoodleBuildRoot\holiday-cn-$tag.zip"))
    {
        Invoke-WebRequest -Uri https: //github.com/NateScarlet/holiday-cn/releases/latest/download/holiday-cn-$tag.zip -OutFile "$DoodleBuildRoot\holiday-cn-$tag.zip"
    }
    Expand-Archive -Path "$DoodleBuildRoot\holiday-cn-$tag.zip" -DestinationPath $DoodleTimePath -Force

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

    &robocopy $DoodleTimePath "$OutPath\dist\time" /MIR /np /njh /njs /ns /nc /ndl /fp /ts
}


Export-ModuleMember -Function Initialize-Doodle