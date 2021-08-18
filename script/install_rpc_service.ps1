$service_name = "doodle_service"
$source_path = "\\192.168.20.59\Users\TD\Source\Doodle\build\doodle\bin"
$source_path_doc = "\\192.168.20.59\Users\TD\Source\Doodle\build\html"
$install_path = "C:\doodle\bin"
$exe_name = "doodleServerExe.exe"
$bin_path = Join-Path $install_path $exe_name


function Update-DoodleFile {
  param (
    $InstallPath,
    $SourcePath
  )
  if (Test-Path $SourcePath) {
    if (Test-Path $InstallPath) {
      Remove-Item -Path $InstallPath -Recurse
    }
    Copy-Item -Path $SourcePath -Destination $InstallPath -Recurse
  }
  
}

Update-DoodleFile -InstallPath ($env:APPDATA + "\Apache24\htdocs") -SourcePath $source_path_doc

$exe = Get-Process | Where-Object { $_.ProcessName -eq "doodleServerExe" }
if ($exe) {
  $exe.CloseMainWindow()
  Write-Host "stop " $service_name
  Write-Host "updata  " $service_name
  Update-DoodleFile -InstallPath $install_path -SourcePath $source_path
  
  Write-Host "start  " $service_name
  Start-Process $bin_path
}
else {
  Write-Host "not find " $service_name
  Write-Host "updata  " $service_name
  Update-DoodleFile -InstallPath $install_path -SourcePath $source_path

  Write-Host "new service  " $service_name $bin_path
  Start-Process -FilePath $bin_path 
}



