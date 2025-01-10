$OutputEncoding = [System.Text.Encoding]::UTF8

Import-Module -Name $PSScriptRoot\DoodlePackageFun.psm1 -Force

$DoodleExePath = "E:\source\doodle\dist\doodle.exe"
$DoodleOut = Convert-Path "$PSScriptRoot/../build"
Initialize-Doodle -OutPath "$DoodleOut/epiboly"
Copy-Item $DoodleExePath -Destination "$DoodleOut/epiboly"
#Compress-Archive -Path "$DoodleOut/epiboly/*" -DestinationPath "$DoodleOut/epiboly.zip" -Force
#$DoodleName = (Get-ChildItem "$DoodleOut/epiboly/dist/" -Filter "*.zip")[0].Name.Split("-")[1]
#Copy-Item "$DoodleOut/epiboly.zip" -Destination "\\192.168.0.67\soft\TD软件\doodle外包\epiboly-Doodle-$DoodleName-win64.zip"