Write-Host " 准备测试服务器 .... 请点击确认按钮并等待完成..."

if (<# (Test-Connection "192.168.10.250" -Count 1 -Delay 1 -Quiet) -and  #>(Test-Path -Path "\\192.168.10.250\public\changanhuanjie")) {
    $Net_MAPS = @(
        @("X:", "\\192.168.10.250\public\changanhuanjie", "长安幻街"),
        @("V:", "\\192.168.10.250\public\DuBuXiaoYao_3", "独步逍遥v3"),
        @("W:", "\\192.168.10.250\public\Prism_projects", "程序开发"),
        @("Y:", "\\192.168.10.250\public\动画共享", "动画共享"),
        @("U:", "\\192.168.10.250\public\WanYuFengShen", "万域封神"),
        # @("T:", "\\192.168.10.250\public\KuangShenMoZun", "狂神魔尊"),
        @("S:", "\\192.168.10.250\public\CangFeng", "藏锋"),
        @("R:", "\\192.168.10.250\public\WanGuXieDi", "万古邪帝"),
        @("Q:", "\\192.168.10.250\public\renjianzuideyi", "人间最得意"),
        @("P:", "\\192.168.10.250\public\WanGuShenHua", "万古神话"),
        @("O:", "\\192.168.10.250\public\11-剪辑", "剪辑"),
        @("N:", "\\192.168.10.250\public\HouQi", "后期"),
        @("T:", "\\192.168.10.218\KuangShenMoZun", "狂神魔尊")
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


$My_Str = "映射完成: `n"
foreach ($nets in $Net_MAPS) {
    if (Test-Path $nets[1]) {
        $My_Str += ($nets[0] + " 映射到: " + $nets[2] + "`n")
        for ($i = 0; $i -lt ($Net_List.Count()); ++$i) {
            if ($Net_List[$i] -eq $nets[0]) {
                $Net.RemoveNetworkDrive($nets[0])
            }
            ++$i
        }
        $Net.MapNetworkDrive($nets[0], $nets[1])
        $rename.NameSpace($nets[0] + "\").Self.Name = $nets[2]
    }
}
$My_Str += "`n`n即将重启文件管理器"
Write-Host $My_Str
Stop-Process -ProcessName explorer
Start-Process explorer
# ps2exe c:\Users\TD\Source\Doodle\script\Cmd_tool\map_net_disk.ps1 c:\Users\TD\Source\Doodle\script\Cmd_tool\run2.exe -noConsole