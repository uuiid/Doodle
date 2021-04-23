Write-Host("正在测试服务器 .... 请等待..")

if ( (Test-Connection "192.168.10.250" -Count 1 -Delay 1 -Quiet) -and (Test-Path -Path "\\192.168.10.250\public\changanhuanjie") ) {
  $Net_MAPS = @(
    @("X:", "\\192.168.10.250\public\changanhuanjie", "长安幻街"),
    @("V:", "\\192.168.10.250\public\DuBuXiaoYao_3", "独步逍遥v3"),
    @("W:", "\\192.168.10.250\public\Prism_projects", "程序开发"),
    @("Y:", "\\192.168.10.250\public\动画共享", "动画共享"),
    @("U:", "\\192.168.10.250\public\WanYuFengShen", "万域封神"),
    @("T:", "\\192.168.10.250\public\KuangShenMoZun", "狂神魔尊"),
    @("S:", "\\192.168.10.250\public\CangFeng", "藏锋"),
    @("R:", "\\192.168.10.250\public\WanGuXieDi", "万古邪帝"),
    @("Q:", "\\192.168.10.250\public\renjianzuideyi", "人间最得意")
  )
}
elseif ( (Test-Connection "192.168.10.218" -Count 1 -Delay 1 -Quiet) -and (Test-Path -Path "\\192.168.10.218\changanhuanjie")) {
  $Net_MAPS = @(
    @("X:", "\\192.168.10.218\changanhuanjie", "长安幻街"),
    @("U:", "\\192.168.10.218\WanYuFengShen", "万域封神"),
    @("T:", "\\192.168.10.218\KuangShenMoZun", "狂神魔尊"),
    @("Z:", "\\192.168.10.57\netDir", "苗雨共享")
  )
}
else {
  exit
}
Write-Host("测试完成:")
$Net = New-Object -ComObject WScript.Network
$Net_List = $Net.EnumNetworkDrives();
$rename = new-object -ComObject Shell.Application

foreach ($nets in $Net_MAPS) {
  Write-Host($nets[0] + " 映射到: " + $nets[2])
  for ($i = 0; $i -lt ($Net_List.Count()); ++$i) {
    if ($Net_List[$i] -eq $nets[0]) {
      $Net.RemoveNetworkDrive($nets[0])
    }
    ++$i
  }

  $Net.MapNetworkDrive($nets[0], $nets[1])
  $rename.NameSpace($nets[0] + "\").Self.Name = $nets[2]
}

Stop-Process -ProcessName explorer
Start-Process explorer