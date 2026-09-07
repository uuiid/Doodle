param(
    [string]$EngineRoot = "E:\UnrealEngine",
    [string]$ArchiveDirectory = ""
)

$ErrorActionPreference = "Stop"

# 使用引擎侧 junction 路径（in-tree 程序），而非直接路径 E:\Doodle\DoodleLiveLink
$ProjectFile = Join-Path $EngineRoot "Engine\Source\Programs\DoodleLiveLink\DoodleLiveLink.uproject"
if (-not (Test-Path $ProjectFile)) { throw "找不到项目文件（junction）: $ProjectFile" }

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

# 清理上次构建产物。in-tree 程序会把 Source/*.Target.cs 归档/暂存到项目目录下，
# 若不删除，下一次 UBT 会递归扫描到重复的 Target 定义而报 CS0101。
$StagedBuildsDir = Join-Path $PSScriptRoot "Saved\StagedBuilds"
if (Test-Path $StagedBuildsDir) { Remove-Item $StagedBuildsDir -Recurse -Force }
if (Test-Path $ArchiveDirectory) { Remove-Item $ArchiveDirectory -Recurse -Force }

# DoodleLiveLink 是 in-tree 编辑器程序，没有游戏目标，使用自定义 UAT 命令
# （BuildScript/DoodleLiveLink.Automation.cs，经主 junction 暴露为 Engine/Source/Programs/DoodleLiveLink/BuildScript/）
& $RunUAT MakeDoodleLiveLinkEditor `
    -project="$ProjectArg" `
    -platform=Win64 `
    -clientconfig=Development `
    -build -cook -stage -archive `
    -archivedirectory="$ArchiveArg" `
    -unattended -nop4

exit $LASTEXITCODE
