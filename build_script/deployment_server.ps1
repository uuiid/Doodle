param (
    [switch]$CopyServer
)

#Set-Item WSMan:\localhost\Client\TrustedHosts -Value '*'
#Set-Item WSMan:\localhost\Client\Auth\Basic -Value True
#Set-Item WSMan:\localhost\Service\Auth\Basic -Value True
#Set-Item WSMan:\localhost\Client\AllowUnencrypted -Value True
#Set-Item WSMan:\localhost\Service\AllowUnencrypted -Value True
# Get-ExecutionPolicy
# Set-ExecutionPolicy RemoteSigned

class OffDays
{
    [string]$name
    [string]$date
    [bool]$isOffDay
}

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

$OutputEncoding = [System.Text.Encoding]::UTF8

$DoodleSource = Convert-Path "$PSScriptRoot/../build/Ninja_release/_CPack_Packages/win64/ZIP"
$DoodleName = (Get-ChildItem $DoodleSource -Directory)[0].Name
$DoodleTimePath = "E:\source\kitsu\dist\time"
Write-Host "开始复制文件"
&robocopy "$DoodleSource\$DoodleName\bin" "\\192.168.40.181\tmp\bin" /MIR /np /njh /njs /ns /nc /ndl /fp /ts
&robocopy "E:\source\kitsu\dist" "\\192.168.40.181\tmp\dist" /MIR /np /njh /njs /ns /nc /ndl /fp /ts

Copy-Item "$DoodleSource/$DoodleName.zip" -Destination "\\192.168.40.181\tmp\dist"
Set-Content -Path "\\192.168.40.181\tmp\dist\version.txt" -Value $DoodleName.Split("-")[1] -NoNewline

Copy-Item "E:\source\doodle\dist\doodle.exe" -Destination "\\192.168.40.181\tmp\dist"

# 从github 下载网络资源
# 先查看标签, 再组合下载url
$tag = (Invoke-WebRequest -Uri https://api.github.com/repos/NateScarlet/holiday-cn/tags -Headers @{ "accept" = "application/json" } |
        ConvertFrom-Json)[0].name
# 检查文件是否存在
if (-not (Test-Path "E:\doodle\build\holiday-cn-$tag.zip"))
{
    Invoke-WebRequest -Uri https://github.com/NateScarlet/holiday-cn/releases/latest/download/holiday-cn-$tag.zip -OutFile "E:\doodle\build\holiday-cn-$tag.zip"
}
Expand-Archive -Path "E:\doodle\build\holiday-cn-$tag.zip" -DestinationPath $DoodleTimePath -Force

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

&robocopy $DoodleTimePath "\\192.168.40.181\tmp\dist\time" /MIR /np /njh /njs /ns /nc /ndl /fp /ts

$RootPassword = ConvertTo-SecureString "root" -AsPlainText -Force
$Credential = New-Object -TypeName System.Management.Automation.PSCredential -ArgumentList auto_light,$RootPassword
#Enter-PSSession -ComputerName 192.168.40.181 -Credential $Credential -Authentication Basic
Invoke-Command -ComputerName 192.168.40.181 -Credential $Credential -Authentication Basic -ScriptBlock {
    #    Compare-Object -ReferenceObject (Get-Content -Path "D:\tmp\bin\file_association_http.exe") -DifferenceObject (Get-Content -Path "D:\kitsu\bin\file_association_http.exe")
    $Target = "D:\kitsu"
    $Tmp = "D:\tmp"
    if ($Using:CopyServer -and ((Get-FileHash "$Target\bin\doodle_kitsu_supplement.exe").Hash -ne (Get-FileHash "$Tmp\bin\doodle_kitsu_supplement.exe").Hash))
    {
        Write-Host "更新服务"
        Stop-Service -Force -Name doodle_kitsu_supplement
        &robocopy "$Tmp\bin" "$Target\bin" /MIR /np /njh /njs /ns /nc /ndl /fp /ts
        Start-Service -Name doodle_kitsu_supplement
    }
    &robocopy "$Tmp\dist" "$Target\dist" /MIR /np /njh /njs /ns /nc /ndl /fp /ts
}

