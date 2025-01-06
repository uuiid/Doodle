$OutputEncoding = [System.Text.Encoding]::UTF8

Import-Module -Name $PSScriptRoot\DoodlePackageFun.psm1 -Force

$DoodleExePath = "E:\source\doodle\dist\doodle.exe"
$DoodleOut = Convert-Path "$PSScriptRoot/../build"
Initialize-Doodle -OutPath "$DoodleOut/epiboly"
Copy-Item $DoodleExePath -Destination "$DoodleOut/epiboly"
Compress-Archive -Path "$DoodleOut/epiboly/*" -DestinationPath "$DoodleOut/epiboly.zip" -Force