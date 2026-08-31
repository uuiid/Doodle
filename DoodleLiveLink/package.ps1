param(
    [string]$EngineRoot = "E:\UnrealEngine",
    [string]$ArchiveDirectory = ""
)

$ErrorActionPreference = "Stop"

$ProjectFile = Join-Path $PSScriptRoot "DoodleLiveLink.uproject"
if (-not (Test-Path $ProjectFile)) { throw "找不到项目文件: $ProjectFile" }

$RunUAT = Join-Path $EngineRoot "Engine\Build\BatchFiles\RunUAT.bat"
if (-not (Test-Path $RunUAT)) { throw "找不到 RunUAT: $RunUAT" }

if (-not $ArchiveDirectory) {
    $ArchiveDirectory = Join-Path $PSScriptRoot "Release"
}

# UAT 需要正斜杠路径
$ProjectArg = $ProjectFile.Replace('\', '/')
$ArchiveArg = $ArchiveDirectory.Replace('\', '/')

Write-Host "打包 DoodleLiveLink (Cooked Editor)"
Write-Host "  项目: $ProjectArg"
Write-Host "  输出: $ArchiveArg"

& $RunUAT MakeCookedEditor `
    -project="$ProjectArg" `
    -platform=Win64 `
    -clientconfig=Development `
    -cook -stage -archive `
    -archivedirectory="$ArchiveArg" `
    -unattended -nop4

exit $LASTEXITCODE
