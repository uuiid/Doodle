$OutputEncoding = [System.Text.Encoding]::UTF8

Import-Module $PSScriptRoot/DoodlePackageFun.ps1

$DoodleExePath = "E:\source\doodle\dist\doodle.exe"
$DoodleOut = Convert-Path "$PSScriptRoot/../build"
Package-Doodle -OutPath "$DoodleOut/epiboly"
Copy-Item $DoodleExePath -Destination "$DoodleOut/epiboly"
Compress-Archive -Path "$DoodleOut/epiboly/*" -DestinationPath "$DoodleOut/epiboly.zip" -Force