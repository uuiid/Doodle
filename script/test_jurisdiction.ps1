# \\192.168.10.250\public\权限检查


$Paths = @(
  "\\192.168.10.250\public\DuBuXiaoYao_3\6-moxing",
  "\\192.168.10.240\public\WanGuShenHua\6-moxing",
  "\\192.168.10.218\WanYuFengShen\6-moxing")

class InfoData {
  [string] $Path;
  [bool] $Read;
  [bool] $Remove;
  [string] $User;
  InfoData(  [string] $in_Path,
    [bool] $in_Read,
    [bool] $in_Remove,
    [string] $in_User ) {
    $this.Path = $in_Path;
    $this.Read = $in_Read;
    $this.Remove = $in_Remove;
    $this.User = $in_User;
  }
}


$IP = Get-NetIPAddress | Where-Object { $_.IPAddress -like "192*" }

$User = Read-Host "请输入姓名: "
$_File_IP_Name_ = $IP.IPAddress + "_" + $User
$Infos = @()
foreach ($path_i in $Paths) {
  $Name = New-Guid
  try {
    $N = Get-ChildItem -Path $path_i -ErrorAction Stop
  }
  catch {
    $info = [InfoData]::new($path_i, $false, $false, $User)
    $Infos += $info
    Continue;
  }

  try {
    if (Test-Path $path_i) {
      $n = New-Item -ItemType "file" -Path $path_i -Name $Name -ErrorAction Stop
    }
  }
  catch {
    $info = [InfoData]::new($path_i, $false, $false, $User)
    $Infos += $info
    Continue;
  }

    
  try {
    $n = Remove-Item -Path $path_i/$Name -ErrorAction Stop
  }
  catch {
    $info = [InfoData]::new($path_i, $false, $true, $User)
    $Infos += $info
    Continue;
  }
  $info = [InfoData]::new($path_i, $true, $true, $User)
  $Infos += $info
}

$value = $Infos | Format-Table -Property User, Path, Read, Remove | Out-String


Set-Content -Path "\\192.168.10.250\public\权限检查\$_File_IP_Name_.txt" -Value $value -Encoding "unicode" -Force -ErrorAction Stop


# ps2exe c:\Users\TD\Source\Doodle\script\test_jurisdiction.ps1 c:\Users\TD\Source\Doodle\script\test_jurisdiction.ps1.exe -noConsole
