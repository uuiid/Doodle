Write-Host " 准备测试服务器 .... 请点击确认按钮并等待完成..."

if (<# (Test-Connection "192.168.10.250" -Count 1 -Delay 1 -Quiet) -and  #>(Test-Path -Path "\\192.168.10.250\public\changanhuanjie")) {
    $Net_MAPS = @(
        @("X:", "\\192.168.10.250\public\changanhuanjie", "8幢_长安幻街"),
        @("V:", "\\192.168.10.250\public\DuBuXiaoYao_3", "8幢_独步逍遥v3"),
        @("W:", "\\192.168.10.250\public\Prism_projects", "8幢_程序开发"),
        @("Y:", "\\192.168.10.250\public\动画共享", "8幢_动画共享"),
        @("U:", "\\192.168.10.218\WanYuFengShen", "9幢_万域封神"),
        # @("T:", "\\192.168.10.250\public\KuangShenMoZun", "狂神魔尊"),
        @("T:", "\\192.168.10.218\KuangShenMoZun", "9幢_狂神魔尊"),
        @("S:", "\\192.168.10.250\public\CangFeng", "8幢_藏锋"),
        @("R:", "\\192.168.10.250\public\WanGuXieDi", "8幢_万古邪帝"),
        @("Q:", "\\192.168.10.250\public\renjianzuideyi", "8幢_人间最得意"),
        @("P:", "\\192.168.10.250\public\WanGuShenHua", "8幢_万古神话"),
        @("O:", "\\192.168.10.250\public\11-剪辑", "8幢_剪辑"),
        @("N:", "\\192.168.10.250\public\HouQi", "8幢_后期"),
        @("M:", "\\192.168.10.218\jianji", "9幢_剪辑"),
        @("L:", "\\192.168.10.218\houqi", "9幢_后期")
    )
}
# elseif ( (Test-Connection "192.168.10.218" -Count 1 -Delay 1 -Quiet) -and (Test-Path -Path "\\192.168.10.218\changanhuanjie")) {
#     $Net_MAPS = @(
#     )
# }
else {
    exit
}
#写入新的消息
Write-Host("测试完成:")

$Net = New-Object -ComObject WScript.Network
$Net_List = $Net.EnumNetworkDrives();
$rename = new-object -ComObject Shell.Application

Stop-Process -ProcessName explorer

for ($i = 0; $i -lt ($Net_List.Count()); ++$i) {
    $Net.RemoveNetworkDrive($Net_List[$i])
    ++$i
}

$My_Str = "映射完成: `n"
foreach ($nets in $Net_MAPS) {
    if (Test-Path $nets[1]) {
        $My_Str += ($nets[0] + " 映射到: " + $nets[2] + "`n")
        $Net.MapNetworkDrive($nets[0], $nets[1])
        $rename.NameSpace($nets[0] + "\").Self.Name = $nets[2]
    }
}
$My_Str += "`n`n即将重启文件管理器"
Write-Host $My_Str

Start-Process explorer
# ps2exe c:\Users\TD\Source\Doodle\script\Cmd_tool\map_net_disk.ps1 c:\Users\TD\Source\Doodle\script\Cmd_tool\run2.exe -noConsole
