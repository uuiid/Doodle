#Requires -Version 7.0

$ErrorActionPreference = 'Stop'
$OutputEncoding = [System.Text.Encoding]::UTF8

$UEVersion = '5.7'
$UatPath = "D:\Program Files\Epic Games\UE_$UEVersion\Engine\Build\BatchFiles\RunUAT.bat"
$WorkDir = Join-Path $PSScriptRoot '../script/uePlug'
$PluginPath = Join-Path $WorkDir 'Doodle/Doodle.uplugin'
$PackageDir = Join-Path $WorkDir "$UEVersion/Plugins/Doodle"
$ThirdPartySrc = Join-Path $WorkDir 'Doodle/ThirdParty'
$ThirdPartyDst = Join-Path $PackageDir 'ThirdParty'
$BackupRoot = Join-Path $WorkDir "$UEVersion/Plugins"
$DistPlugins = Join-Path $PSScriptRoot '../build/pack/dist/Plugins'
$RobocopyMirrorArgs = '/mir', '/ndl', '/np', '/njh', '/njs', '/ns', '/nc', '/nfl'
$RobocopySuccessExitCodes = 0..7

Import-Module -Name $PSScriptRoot\DoodlePackageFun.psm1 -Force
Add-GitVersionLabel -WorkingDirectory (Join-Path $PSScriptRoot "..")

function Invoke-Native {
  param(
    [Parameter(Mandatory)] [string] $FilePath,
    [string[]] $ArgumentList = @(),
    [int[]] $SuccessExitCodes = @(0)
  )
  & $FilePath @ArgumentList
  $Code = $LASTEXITCODE
  if ($SuccessExitCodes -notcontains $Code) {
    throw "$FilePath 执行失败，退出码 $Code"
  }
}

function Remove-Directory {
  param([Parameter(Mandatory)] [string] $Path)
  if (Test-Path -LiteralPath $Path) {
    Remove-Item -LiteralPath $Path -Recurse -Force
  }
}

$WorkDir = $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($WorkDir)
$PluginPath = $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($PluginPath)
$PackageDir = $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($PackageDir)
$ThirdPartySrc = $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($ThirdPartySrc)
$ThirdPartyDst = $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($ThirdPartyDst)
$BackupRoot = $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($BackupRoot)
$DistPlugins = $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($DistPlugins)

if (-not (Test-Path -LiteralPath $UatPath)) {
  throw "未找到 RunUAT.bat：$UatPath"
}

$TimeStamp = (Get-Date).ToString('yyyy_MM_dd_HH_mm_ss')
$BackupDir = Join-Path $BackupRoot "Doodle_$TimeStamp"

Write-Host "构建插件：$PluginPath" -ForegroundColor Cyan
Invoke-Native -FilePath $UatPath -ArgumentList @(
  'BuildPlugin'
  "-Plugin=$PluginPath"
  '-TargetPlatforms=Win64'
  "-Package=$PackageDir"
  '-Rocket'
) -SuccessExitCodes @(0, 2)

if (-not (Test-Path -LiteralPath $PackageDir)) {
  throw "BuildPlugin 未产出打包目录：$PackageDir"
}

Write-Host "备份打包结果：$BackupDir" -ForegroundColor Cyan
Invoke-Native -FilePath 'robocopy' -ArgumentList (@($PackageDir, $BackupDir) + $RobocopyMirrorArgs) `
  -SuccessExitCodes $RobocopySuccessExitCodes

Write-Host "复制 ThirdParty：$ThirdPartySrc -> $ThirdPartyDst" -ForegroundColor Cyan
Invoke-Native -FilePath 'robocopy' -ArgumentList (@($ThirdPartySrc, $ThirdPartyDst) + $RobocopyMirrorArgs) `
  -SuccessExitCodes $RobocopySuccessExitCodes

Write-Host '清理中间产物' -ForegroundColor Cyan
Remove-Directory (Join-Path $PackageDir 'Intermediate')
Remove-Directory (Join-Path $PackageDir 'Source')
Remove-Directory $DistPlugins

Write-Host "完成：$PackageDir" -ForegroundColor Green
