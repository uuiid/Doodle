Write-Host " 准备测试服务器 .... 请点击确认按钮并等待完成..."


$map_item = @(
    @("C:\sy\ChangAnHuanJie_8", "\\192.168.10.250\public\changanhuanjie", "8幢_长安幻街"),
    @("C:\sy\DuBuXiaoYao_8", "\\192.168.10.250\public\DuBuXiaoYao_3", "8幢_独步逍遥v3"),
    @("C:\sy\ChengXv_8", "\\192.168.10.250\public\Prism_projects", "8幢_程序开发"),
    @("C:\sy\donghuagongxiang_8", "\\192.168.10.250\public\动画共享", "8幢_动画共享"),
    @("C:\sy\CangFeng_8", "\\192.168.10.250\public\CangFeng", "8幢_藏锋"),
    @("C:\sy\WanGuShenHua_8", "\\192.168.10.250\public\WanGuShenHua", "8幢_万古神话"),
    @("C:\sy\RenJianZuiDeYi_8", "\\192.168.10.250\public\renjianzuideyi", "8幢_人间最得意"),
    @("C:\sy\DaiDing_8", "\\192.168.10.250\public\DaiDing", "8幢_待定名称"),
    @("C:\sy\JianJi_8", "\\192.168.10.250\public\11-剪辑", "8幢_剪辑"),
    @("C:\sy\HouQi_8", "\\192.168.10.250\public\HouQi", "8幢_后期"),
    @("C:\sy\MeiYiGiao_8", "\\192.168.10.250\public\美易高", "美易高"),
    @("C:\sy\WanYuFengShen_9", "\\192.168.10.218\WanYuFengShen", "9幢_万域封神"),
    @("C:\sy\KuangShenMoZun_9", "\\192.168.10.218\KuangShenMoZun", "9幢_狂神魔尊"),
    @("C:\sy\JianJi_9", "\\192.168.10.218\jianji", "9幢_剪辑"),
    @("C:\sy\HouQi_9", "\\192.168.10.218\houqi", "9幢_后期")
    
)

foreach ($item in $map_item) {
    try {
        Get-Childitem -Path $item[1] -ErrorAction Stop
        if (Test-Path $item[1]) {
            if (Test-Path $item[0]) {
                (Get-Item -Path $item[0]).Delete()
            }
            New-Item -ItemType SymbolicLink -Path $item[0] -Target $item[1]
        }
    }
    catch [System.Management.Automation.ActionPreferenceStopException] {
        Write-Host "目录" $item[1] "没有访问权限， 取消映射"
    }
    catch {
        Write-Host "catch all 目录 " $item[1] " 没有访问权限， 取消映射"
    }
}
# ps2exe c:\Users\TD\Source\Doodle\script\Cmd_tool\map_sysDir.ps1 c:\Users\TD\Source\Doodle\script\Cmd_tool\run4.exe -requireAdmin
# try {
#     Get-Childitem -Path \\192.168.10.218\zhipian -ErrorAction Stop
# }
# catch [System.UnauthorizedAccessException]{
#     Write-Host "not file"
# }
# Get-Childitem -Path \\192.168.10.218\houqi 