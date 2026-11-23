Import-Module -Name $PSScriptRoot\DoodlePackageFun.psm1 -Force

Add-GitVersionLabel -WorkingDirectory (Join-Path $PSScriptRoot "..")
& "$PSScriptRoot\build\set_env.ps1" C:/PROGRA~1/CMake/bin/cmake.exe --workflow --preset build_release
if ($LASTEXITCODE -ne 0) { throw "release 构建失败，退出码 $LASTEXITCODE" }

. "$PSScriptRoot\build\copy_to_share.ps1"