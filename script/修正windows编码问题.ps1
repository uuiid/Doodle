# 定义源目录和目标目录
$sourceRoot = "C:\Program Files\Autodesk\Maya2024"
$targetRoot = "D:\Program Files\Autodesk\Maya2024"
$targetBackup = "D:\Program Files\Autodesk\Maya2024_backup"

# 确保目标根目录存在
if (-not (Test-Path -Path $targetRoot)) {
  New-Item -Path $targetRoot -ItemType Directory -Force | Out-Null
}

# 递归获取所有 .mel 文件（包括子文件夹）
Get-ChildItem -Path $sourceRoot -Recurse -Filter "*.mel" -File | ForEach-Object {
  $sourceFile = $_.FullName

  # 计算相对路径（相对于源根目录）
  $relativePath = $sourceFile.Substring($sourceRoot.Length).TrimStart('\')
  $targetFile = Join-Path -Path $targetRoot -ChildPath $relativePath
  $targetFileBackup = Join-Path -Path $targetBackup -ChildPath $relativePath

  # 确保目标文件的父目录存在
  $targetDir = Split-Path -Path $targetFile -Parent
  if (-not (Test-Path -Path $targetDir)) {
    New-Item -Path $targetDir -ItemType Directory -Force | Out-Null
  }

  $targetBackupDir = Split-Path -Path $targetFileBackup -Parent
  if (-not (Test-Path -Path $targetBackupDir)) {
    New-Item -Path $targetBackupDir -ItemType Directory -Force | Out-Null
  }

  # 直接复制
  Copy-Item -Path $sourceFile -Destination $targetFile -Force
  Copy-Item -Path $sourceFile -Destination $targetFileBackup -Force

  # 读取文件内容（使用 GB2312 编码）
  $content = Get-Content -Path $sourceFile -Raw -Encoding "GB2312"

  # 写入目标文件（使用 UTF-8 编码，无 BOM）
  $content | Set-Content -Path $targetFile -Encoding "UTF8"

  # 可选：输出进度信息
  Write-Host "Converted: $sourceFile -> $targetFile"
}

Write-Host "All .mel files converted."