$service_name = "doodle_service"
$source_path = "\\192.168.20.59\Users\TD\Source\Doodle\b_Ninja_VS19_Ni_64_qt515_Release\bin"
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
$exe = Get-Process | Where-Object {$_.ProcessName -eq "doodleServerExe"}
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



