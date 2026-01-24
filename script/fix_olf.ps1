$root = "D:\kitsu\images\pictures\thumbnails\squ\are"

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

    $l_file_tem_str = $newPath.Split("\")[-1]
    $filename = "$l_file_tem_str.png"
    $newPath = "$($newPath.Substring(0,$l_file_tem_str.Length - 1 ))\$($l_file_tem_str.Substring(0,3))\$($l_file_tem_str.Substring(3,3))"
    if (-not (Test-Path -Path $newPath)) {
      # 创建目录
      New-Item -ItemType Directory -Path $newPath -Force
    }
    $newPath = "$newPath\$filename"
  }
  else {
    $newPath = "$line.png"
  }
  if ( Test-Path -Path $newPath) {
    Write-Host "File $newPath already exists, skipping..."
    continue
  }
  # Write-Host "Renaming $line to $newPath"
  Move-Item -Path $line -Destination $newPath
}