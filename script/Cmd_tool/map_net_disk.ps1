$Net = New-Object -ComObject WScript.Network
$Net.RemoveNetworkDrive("X:")
$Net.RemoveNetworkDrive("V:")
$Net.RemoveNetworkDrive("W:")
$Net.RemoveNetworkDrive("Y:")
$Net.RemoveNetworkDrive("U:")
$Net.RemoveNetworkDrive("T:")
$Net.RemoveNetworkDrive("S:")
$Net.RemoveNetworkDrive("R:")
$Net.RemoveNetworkDrive("Q:")

# Stop-Process -ProcessName explorer
# Start-Process explorer
$Net.MapNetworkDrive("X:", '\\192.168.10.250\public\changanhuanjie')
$Net.MapNetworkDrive("Y:", '\\192.168.10.250\public\动画共享')
$Net.MapNetworkDrive("W:", '\\192.168.10.250\public\Prism_projects')
$Net.MapNetworkDrive("V:", '\\192.168.10.250\public\DuBuXiaoYao_3')
$Net.MapNetworkDrive("U:", '\\192.168.10.250\public\WanYuFengShen')
$Net.MapNetworkDrive("T:", '\\192.168.10.250\public\KuangShenMoZun')
$Net.MapNetworkDrive("S:", '\\192.168.10.250\public\CangFeng')
$Net.MapNetworkDrive("R:", '\\192.168.10.250\public\WanGuXieDi')
$Net.MapNetworkDrive("Q:", '\\192.168.10.250\public\renjianzuideyi')

$rename = new-object -ComObject Shell.Application
$rename.NameSpace("X:\").Self.Name = "长安幻街"
$rename.NameSpace("V:\").Self.Name = "独步逍遥v3"
$rename.NameSpace("U:\").Self.Name = "万域封神"
$rename.NameSpace("T:\").Self.Name = "狂神魔尊"
$rename.NameSpace("W:\").Self.Name = "程序开发"
$rename.NameSpace("Y:\").Self.Name = "动画共享"
$rename.NameSpace("S:\").Self.Name = "藏锋"
$rename.NameSpace("R:\").Self.Name = "万古邪帝"
$rename.NameSpace("Q:\").Self.Name = "人间最得意"
Stop-Process -ProcessName explorer
Start-Process explorer