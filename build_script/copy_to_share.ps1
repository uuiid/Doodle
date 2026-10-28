param (
  [string]$ShareRoot = "\\192.168.20.88\分享",
  [string]$BuildDir = "$PSScriptRoot/../build",
  [string]$ApiDir = "$PSScriptRoot/../docs/api",
  [string]$DbName = "kitsu_new.db",
  [string]$TimeStamp = (Get-Date -Format "yyyyMMdd_HHmmss")
)

$ErrorActionPreference = "Stop"

$l_build_dir = [System.IO.Path]::GetFullPath($BuildDir)
$l_api_dir = [System.IO.Path]::GetFullPath($ApiDir)
$l_install_dir = Join-Path $l_build_dir "install"
$l_db_file = Join-Path $l_build_dir $DbName
$l_destination = Join-Path $ShareRoot $TimeStamp

function Copy-ToTarget {
  param (
    [string]$Source,
    [string]$Destination,
    [switch]$Recurse
  )
  Write-Host "复制 $Source -> $Destination"
  Copy-Item -LiteralPath $Source -Destination $Destination -Recurse:$Recurse -Force
}

$l_missing = @()
if (-not (Test-Path -LiteralPath $l_install_dir -PathType Container)) { $l_missing += $l_install_dir }
if (-not (Test-Path -LiteralPath $l_db_file -PathType Leaf)) { $l_missing += $l_db_file }
if (-not (Test-Path -LiteralPath $l_api_dir -PathType Container)) { $l_missing += $l_api_dir }
if ($l_missing.Count -gt 0) {
  foreach ($l_item in $l_missing) { Write-Host "源不存在: $l_item" }
  exit 1
}

if (-not (Test-Path -LiteralPath $l_destination -PathType Container)) {
  Write-Host "创建目标目录 $l_destination"
  New-Item -ItemType Directory -Path $l_destination -Force | Out-Null
}

Copy-ToTarget -Source $l_install_dir -Destination $l_destination -Recurse

$l_db_target = Join-Path $l_destination $DbName
Copy-ToTarget -Source $l_db_file -Destination $l_db_target
foreach ($l_suffix in @("-wal", "-shm")) {
  $l_source = "$l_db_file$l_suffix"
  if (-not (Test-Path -LiteralPath $l_source -PathType Leaf)) {
    Write-Host "跳过不存在的 $l_source"
    continue
  }
  Copy-ToTarget -Source $l_source -Destination "$l_db_target$l_suffix"
}

$l_api_target = Join-Path $l_destination "api"
Copy-ToTarget -Source $l_api_dir -Destination $l_api_target -Recurse

Write-Host "复制完成 $l_destination"
