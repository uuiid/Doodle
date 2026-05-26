$Root = "E:\"
$DoodleRoot = "E:\Doodle"
$KitsuRoot = "E:\source\kitsu"
$SDRoot = "E:\source\SD"

function Create-GitArchive {
  param (
    [string]$RepoPath,
    [string]$OutputPath
  )
  if (Test-Path $OutputPath) {
    Remove-Item $OutputPath -Force
  }
  Write-Host "正在打包 $RepoPath 的源代码..."

  Start-Process -FilePath "git.exe" -ArgumentList archive, "--format=zip", "--output=$OutputPath", "HEAD" -WorkingDirectory $RepoPath -NoNewWindow -Wait
  Write-Host "源代码已打包到 $OutputPath"
}
Create-GitArchive -RepoPath $DoodleRoot -OutputPath "$Root\doodle_source.zip"
Create-GitArchive -RepoPath $KitsuRoot -OutputPath "$Root\kitsu_source.zip"
Create-GitArchive -RepoPath $SDRoot -OutputPath "$Root\sd_source.zip"

if (Test-Path "$Root\source_code.zip") {
  Remove-Item "$Root\source_code.zip" -Force
}

Compress-Archive -Path "$Root\doodle_source.zip", "$Root\kitsu_source.zip", "$Root\sd_source.zip" -DestinationPath "$Root\source_code.zip" -Force
Remove-Item "$Root\doodle_source.zip", "$Root\kitsu_source.zip", "$Root\sd_source.zip" -Force