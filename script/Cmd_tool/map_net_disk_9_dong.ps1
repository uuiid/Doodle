$Net = New-Object -ComObject WScript.Network
$Net.RemoveNetworkDrive("X:")
# $Net.RemoveNetworkDrive("V:")
# $Net.RemoveNetworkDrive("W:")
# $Net.RemoveNetworkDrive("Y:")
$Net.RemoveNetworkDrive("U:")
$Net.RemoveNetworkDrive("T:")
$Net.RemoveNetworkDrive("Z:")

# Stop-Process -ProcessName explorer
# Start-Process explorer
$Net.MapNetworkDrive("X:", '\\192.168.10.218\changanhuanjie')
$Net.MapNetworkDrive("T:", '\\192.168.10.218\KuangShenMoZun')
$Net.MapNetworkDrive("U:", '\\192.168.10.218\WanYuFengShen')
# $Net.MapNetworkDrive("Y:", '\\192.168.10.218\动画共享')
# $Net.MapNetworkDrive("W:", '\\192.168.10.218\Prism_projects')
# $Net.MapNetworkDrive("V:", '\\192.168.10.218\DuBuXiaoYao_3')
$Net.MapNetworkDrive("Z:", '\\192.168.10.57\netDir')

$rename = new-object -ComObject Shell.Application
$rename.NameSpace("X:\").Self.Name = "长安幻街"
$rename.NameSpace("T:\").Self.Name = "狂神魔尊"
$rename.NameSpace("U:\").Self.Name = "万域封神"
# $rename.NameSpace("V:\").Self.Name = "独步逍遥v3"
# $rename.NameSpace("W:\").Self.Name = "程序开发"
# $rename.NameSpace("Y:\").Self.Name = "动画共享"
$rename.NameSpace("Z:\").Self.Name = "苗雨共享"
Stop-Process -ProcessName explorer
Start-Process explorer