$OutputEncoding = [System.Text.Encoding]::UTF8

Import-Module -Name $PSScriptRoot\DoodlePackageFun.psm1 -Force
$Tags = git tag --sort=-v:refname;
# 去除 前缀 v
$Tags = $Tags | ForEach-Object { $_ -replace "v", "" }
$DoodleVersion = $Tags[0]
Write-Host "Packaging Doodle version $DoodleVersion for Epiboly"

$DoodleExePath = "E:\source\doodle\dist\win-unpacked\*"
$DoodleOut = Convert-Path "$PSScriptRoot/../build"
Remove-Item -Path "$DoodleOut/epiboly/*" -Recurse -Force
Initialize-Doodle -OutPath "$DoodleOut/epiboly" -OnlyOne

Copy-Item $DoodleExePath -Destination "$DoodleOut/epiboly" -Recurse -Force
Expand-Archive -Path "$DoodleOut/Ninja_release/Doodle-$DoodleVersion-win64.zip" -DestinationPath "$DoodleOut/epiboly" -Force
Copy-Item -Path "$DoodleOut/epiboly/Doodle-$DoodleVersion-win64/maya" -Destination "$DoodleOut/epiboly/maya" -Recurse -Force
Remove-Item -Path "$DoodleOut/epiboly/Doodle-$DoodleVersion-win64" -Recurse -Force
Remove-Item -Path "$DoodleOut/epiboly" -Include "*.zip" -Recurse -Force
Remove-Item -Path "$DoodleOut/epiboly/dist" -Include "*.exe" -Recurse -Force

$DataSource = "$DoodleOut/epiboly/epiboly.database"
Copy-Item "\\192.168.40.181\Dev\kitsu_new.database" $DataSource -Force
$Tabls = Invoke-SqliteQuery -DataSource $DataSource -Query "SELECT name FROM sqlite_master WHERE type='table';"
foreach ($Tab in $Tabls) {
    if ($Tab.name -eq "sqlite_sequence") { continue }
    if ($Tab.name -eq "project") { continue }
    if ($Tab.name -eq "project_status") { continue }
    Invoke-SqliteQuery -DataSource $DataSource -Query "DROP TABLE $($Tab.name);"
}
Invoke-SqliteQuery -DataSource $DataSource -Query "VACUUM;"

Compress-Archive -Path "$DoodleOut/epiboly/*" -DestinationPath "$DoodleOut/epiboly.zip" -Force

$DoodleVersionSuf = 0

while (Test-Path -Path "\\192.168.0.67\soft\TD软件\doodle外包\epiboly-Doodle-$DoodleVersion.$DoodleVersionSuf-win64.zip") {
    $DoodleVersionSuf += 1
    Write-Host "\\192.168.0.67\soft\TD软件\doodle外包\epiboly-Doodle-$DoodleVersion.$DoodleVersionSuf-win64.zip";
}

Copy-Item "$DoodleOut/epiboly.zip" -Destination "\\192.168.0.67\soft\TD软件\doodle外包\epiboly-Doodle-$DoodleVersion.$DoodleVersionSuf-win64.zip" -Force