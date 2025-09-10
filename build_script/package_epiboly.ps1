$OutputEncoding = [System.Text.Encoding]::UTF8

Import-Module -Name $PSScriptRoot\DoodlePackageFun.psm1 -Force

$DoodleExePath = "E:\source\doodle\dist\win-unpacked\*"
$DoodleOut = Convert-Path "$PSScriptRoot/../build"
Remove-Item -Path "$DoodleOut/epiboly" -Include "*.zip" -Recurse -Force

Initialize-Doodle -OutPath "$DoodleOut/epiboly" -OnlyOne
Copy-Item $DoodleExePath -Destination "$DoodleOut/epiboly" -Recurse -Force
Remove-Item -Path "$DoodleOut/epiboly/dist/Plugins/*.zip" -Force

Compress-Archive -Path "$DoodleOut/epiboly/*" -DestinationPath "$DoodleOut/epiboly.zip" -Force
$DoodleName = (Get-ChildItem "$DoodleOut/epiboly/dist/" -Filter "*.zip")[-1].Name.Split("-")[1] + ".0"
$DoodleVersion = 0

while (Test-Path -Path "\\192.168.0.67\soft\TD软件\doodle外包\epiboly-Doodle-$DoodleName.$DoodleVersion-win64.zip")
{
    $DoodleVersion += 1
    Write-Host "\\192.168.0.67\soft\TD软件\doodle外包\epiboly-Doodle-$DoodleName.$DoodleVersion-win64.zip";
}

Copy-Item "$DoodleOut/epiboly.zip" -Destination "\\192.168.0.67\soft\TD软件\doodle外包\epiboly-Doodle-$DoodleName.$DoodleVersion-win64.zip" -Force