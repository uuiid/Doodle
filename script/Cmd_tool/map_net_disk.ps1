$Net = New-Object -ComObject WScript.Network
$Net.RemoveNetworkDrive("X:")
$Net.RemoveNetworkDrive("V:")
$Net.RemoveNetworkDrive("U:")
$Net.RemoveNetworkDrive("T:")


$Net.MapNetworkDrive("Y:",'\\192.168.10.253\动画共享')
$Net.MapNetworkDrive("Z:",'\\192.168.10.253\动画共享')
$Net.MapNetworkDrive("W:",'\\192.168.10.253\Prism_projects')
$Net.MapNetworkDrive("X:",'\\192.168.10.253\changanhuanjie')
$Net.MapNetworkDrive("V:",'\\192.168.10.253\DuBuXiaoYao_3')
$Net.MapNetworkDrive("U:",'\\192.168.10.253\WanYuFengShen')
$Net.MapNetworkDrive("T:",'\\192.168.10.253\KuangShenMoZun')

$rename = new-object -ComObject Shell.Application
$rename.NameSpace("X:\").Self.Name = "长安幻街"
$rename.NameSpace("V:\").Self.Name = "独步逍遥v3"
$rename.NameSpace("U:\").Self.Name = "万域封神"
$rename.NameSpace("T:\").Self.Name = "狂神魔尊"
$rename.NameSpace("W:\").Self.Name = "程序开发"