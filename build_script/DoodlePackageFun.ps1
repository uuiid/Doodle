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
        $Json.days += $day
    }
    $Json | ConvertTo-Json  | Set-Content -Path $Path -Encoding UTF8
}


function Package-Doodle
{
    param(
        [string]$OutPath
    )
    if (-not (Test-Path $OutPath))
    {
        New-Item $OutPath -ItemType Directory
    }
    $DoodleBuildRoot = Convert-Path "$PSScriptRoot/../build"
    $DoodleSource = Convert-Path "$PSScriptRoot/../build/Ninja_release/_CPack_Packages/win64/ZIP"
    $DoodleName = (Get-ChildItem $DoodleSource -Directory)[0].Name
    $DoodleKitsuRoot = "E:\source\kitsu"
    $DoodleTimePath = "$DoodleKitsuRoot\dist\time"
    $DoodleExePath = "E:\source\doodle\dist\doodle.exe"

    Write-Host "开始复制文件"
    &robocopy "$DoodleSource\$DoodleName\bin" "$OutPath\bin" /MIR /np /njh /njs /ns /nc /ndl /fp /ts
    &robocopy "$DoodleKitsuRoot\dist" "$OutPath\dist" /MIR /np /njh /njs /ns /nc /ndl /fp /ts

    Copy-Item "$DoodleSource/$DoodleName.zip" -Destination "$OutPath\dist"
    Set-Content -Path "$OutPath\dist\version.txt" -Value $DoodleName.Split("-")[1] -NoNewline

    Copy-Item $DoodleExePath -Destination "$OutPath\dist"

    # 从github 下载网络资源
    # 先查看标签, 再组合下载url
    $tag = (Invoke-WebRequest -Uri https://api.github.com/repos/NateScarlet/holiday-cn/tags -Headers @{ "accept" = "application/json" } |
            ConvertFrom-Json)[0].name
    # 检查文件是否存在
    if (-not (Test-Path "$DoodleBuildRoot\holiday-cn-$tag.zip"))
    {
        Invoke-WebRequest -Uri https://github.com/NateScarlet/holiday-cn/releases/latest/download/holiday-cn-$tag.zip -OutFile "$DoodleBuildRoot\holiday-cn-$tag.zip"
    }
    Expand-Archive -Path "$DoodleBuildRoot\holiday-cn-$tag.zip" -DestinationPath $DoodleTimePath -Force

    Add-Compensatory -Path "$DoodleTimePath\2024.json" @(
        [System.Management.Automation.PSObject]@{
            name = "公司年假补班"
            date = "2024-12-14"
            isOffDay = $true
        },
        [System.Management.Automation.PSObject]@{
            name = "公司年假补班"
            date = "2024-12-29"
            isOffDay = $true
        }
    )

    Add-Compensatory -Path "$DoodleTimePath\2025.json" @(
        [System.Management.Automation.PSObject]@{
            name = "公司年假补班"
            date = "2024-01-11"
            isOffDay = $true
        }
    )

    &robocopy $DoodleTimePath "$OutPath\dist\time" /MIR /np /njh /njs /ns /nc /ndl /fp /ts
}


