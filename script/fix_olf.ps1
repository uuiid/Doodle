$root = "D:\kitsu_data"

$files = "D:\old_files.txt"
if (-not (Test-Path $files)) {
  # 迭代所有文件
  Get-ChildItem -Path $root -Recurse -File | Where-Object { $_.Extension -eq "" } | ForEach-Object {
    # 写入文件路径到文本文件
    Add-Content -Path $files -Value $_.FullName
  }
}

$lines = Get-Content -Path $files
foreach ($line in $lines) {
  # 文件名称以 square- 开头的，特殊处理
  $newPath = ""
  if ($line -like "*square-*") {
    $newPath = $line.Replace("squ\are\square-", "")

    $filename = "$($newPath.Substring(34,3))\$($newPath.Substring(37,3))\$($newPath.Substring(34)).png"
    $newPath = "$($newPath.Substring(0,$newPath.Length-37))"
    if (-not (Test-Path $newPath)) {
      # 创建目录
      New-Item -ItemType Directory -Path $newPath -Force
    }
    $newPath = "$newPath\$filename"
  }
  else {
    $newPath = "$line.png"
  }
  if ( Test-Path $newPath) {
    Write-Host "File $newPath already exists, skipping..."
    continue
  }
  Write-Host "Renaming $line to $newPath"
  # Rename-Item -Path $line -NewName $newPath
}