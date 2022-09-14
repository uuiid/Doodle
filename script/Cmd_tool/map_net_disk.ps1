Write-Host " 准备测试服务器 .... 请点击确认按钮并等待完成..."
# 这个是老版本的脚本， 基本上只有稍微维护一下就行

$Net_MAPS = @(
    @("V:", "\\192.168.10.250\public\DuBuXiaoYao_3", "独步逍遥v3_250"),
    @("Z:", "\\192.168.10.240\public\MeYiGao2", "美易高2"),
    @("U:", "\\192.168.10.218\WanYuFengShen", "万域封神_218"),
    @("T:", "\\192.168.10.218\KuangShenMoZun", "狂神魔尊_218"),
    @("S:", "\\192.168.10.240\public\CangFeng", "藏锋_240"),
    @("R:", "\\192.168.10.240\public\WanGuShenHua", "万古神话_240"),
    @("Q:", "\\192.168.10.250\public\renjianzuideyi", "人间最得意_250"),
    @("O:", "\\192.168.10.240\public\剪辑_240", "剪辑_240"),
    @("N:", "\\192.168.10.250\public\HouQi", "后期_250"),
    @("M:", "\\192.168.10.218\jianji", "剪辑_218"),
    @("L:", "\\192.168.10.218\houqi", "后期_218"),
    @("K:", "\\192.168.10.250\public\美易高", "美易高_250"),
    @("W:", "\\192.168.10.240\public\mygwaibao", "美易高_外包") # ,
   # @("Y:", "\\192.168.10.240\public\mygwaibao2", "美易高_外包2")
)

#写入新的消息
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
    try {
        Get-Childitem -Path $nets[1] -ErrorAction Stop
        if (Test-Path $nets[1]) {
            $My_Str += ($nets[0] + " 映射到: " + $nets[2] + "`n")
            $Net.MapNetworkDrive($nets[0], $nets[1])
            $rename.NameSpace($nets[0] + "\").Self.Name = $nets[2]
        }
    }
    catch [System.Management.Automation.ActionPreferenceStopException] {
        Write-Host "目录" $nets[1] "没有访问权限， 取消映射"
    }
    catch {
        Write-Host "catch all 目录 " $nets[1] " 没有访问权限， 取消映射"
    }
}
$My_Str += "`n`n即将重启文件管理器"
Write-Host $My_Str

Start-Process explorer
# ps2exe c:\Users\TD\Source\Doodle\script\Cmd_tool\map_net_disk.ps1 c:\Users\TD\Source\Doodle\script\Cmd_tool\run2.4.exe
