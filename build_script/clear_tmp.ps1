$tmp_files = Get-ChildItem -Path "$env:TEMP/doodle" -Recurse | Where-Object { $_.LastWriteTime -lt (Get-Date).AddDays(-1) }

foreach ($file in $tmp_files) {
  try {
    if ( -not $file.PSIsContainer) {
      Remove-Item -Path $file.FullName -Force -ErrorAction SilentlyContinue
    }
  }
  catch {
    Write-Host "无法删除文件: $($file.FullName) - 错误: $($_.Exception.Message)"
  }
}