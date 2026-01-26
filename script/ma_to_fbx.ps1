# 列出目录中的.ma文件并转换为.fbx格式
$sourceDir = "D:\test_files\test\new"
$targetDir = "D:\test_files\test\new"

$l_list_ma = Get-ChildItem -Path $sourceDir -Filter *.ma -Recurse

foreach ($file in $l_list_ma) {
  $sourcePath = $file.FullName
  $targetPath = Join-Path -Path $targetDir -ChildPath ($file.BaseName + ".fbx")
  $targetPath = $targetPath.Replace("\", "/")
  $comm_mel = "FBXExport -f `"$targetPath`"; quit;"
  # 将命令写入到临时的 MEL 脚本文件
  $comm_mel_file = "$([System.IO.Path]::GetTempPath())\convert_ma_to_fbx.mel"
  Set-Content -Path $comm_mel_file -Value $comm_mel
  # 调用Maya命令行进行转换
  & "C:\Program Files\Autodesk\Maya2020\bin\mayabatch.exe" -file $sourcePath -script $comm_mel_file

  Write-Host "Converted $sourcePath to $targetPath"
}