$OutputEncoding = [System.Text.Encoding]::UTF8

Import-Module -Name $PSScriptRoot\DoodlePackageFun.psm1 -Force

$DoodleExePath = "E:\source\doodle\dist\win-unpacked\*"
$DoodleOut = Convert-Path "$PSScriptRoot/../build"
Initialize-Doodle -OutPath "$DoodleOut/epiboly" -OnlyOne
Copy-Item $DoodleExePath -Destination "$DoodleOut/epiboly" -Recurse -Force
Compress-Archive -Path "$DoodleOut/epiboly/*" -DestinationPath "$DoodleOut/epiboly.zip" -Force
$DoodleName = (Get-ChildItem "$DoodleOut/epiboly/dist/" -Filter "*.zip")[0].Name.Split("-")[1] + ".0"

do
{
    $Arr = $DoodleName.Split(".")
    # 分解后最后一位递增 1
    $DoodleName = ($Arr[0..($Arr.Length - 2)] | Join-String -Separator ".") + "." + ([Int32]$Arr[-1] + 1)
    Write-Host $DoodleName;
} while (Test-Path -Path "\\192.168.0.67\soft\TD软件\doodle外包\epiboly-Doodle-$DoodleName-win64.zip")

Copy-Item "$DoodleOut/epiboly.zip" -Destination "\\192.168.0.67\soft\TD软件\doodle外包\epiboly-Doodle-$DoodleName-win64.zip" -Force