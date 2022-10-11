Write-Host " 准备测试服务器 .... 请点击确认按钮并等待完成..."
# 这个是老版本的脚本， 基本上只有稍微维护一下就行

$Net_MAPS = @(
@("H:", "\\192.168.10.240\public\Hui.Chen", "Hui.Chen")
)

#写入新的消息
$Net = New-Object -ComObject WScript.Network
$rename = new-object -ComObject Shell.Application

Stop-Process -ProcessName explorer

$My_Str = "映射完成: `n"
foreach ($nets in $Net_MAPS)
{
    try
    {
        Get-Childitem -Path $nets[1] -ErrorAction Stop
        if (Test-Path $nets[1])
        {
            $My_Str += ($nets[0] + " 映射到: " + $nets[2] + "`n")
            $Net.MapNetworkDrive($nets[0], $nets[1])
            $rename.NameSpace($nets[0] + "\").Self.Name = $nets[2]
        }
    }
    catch [System.Management.Automation.ActionPreferenceStopException]
    {
        Write-Host "目录" $nets[1] "没有访问权限， 取消映射"
    }
    catch
    {
        Write-Host "catch all 目录 " $nets[1] " 没有访问权限， 取消映射"
    }
}
$My_Str += "`n`n即将重启文件管理器"
Write-Host $My_Str

Start-Process explorer
# ps2exe E:\Doodle\script\Cmd_tool\map_1.ps1 E:\Doodle\script\Cmd_tool\hui_chen.exe
