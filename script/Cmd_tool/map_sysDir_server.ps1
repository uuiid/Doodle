Write-Host " 准备测试服务器 .... 请点击确认按钮并等待完成..."


$map_item = @(
    @("ChangAnHuanJie", "\\192.168.10.250\public\changanhuanjie", "8幢_长安幻街"),
    @("DuBuXiaoYao", "\\192.168.10.250\public\DuBuXiaoYao_3", "8幢_独步逍遥v3"),
    @("ChengXv", "\\192.168.10.250\public\Prism_projects", "8幢_程序开发"),
    @("donghuagongxiang", "\\192.168.10.250\public\动画共享", "8幢_动画共享"),
    @("CangFeng", "\\192.168.10.250\public\CangFeng", "8幢_藏锋"),
    @("WanGuShenHua", "\\192.168.10.250\public\WanGuShenHua", "8幢_万古神话"),
    @("RenJianZuiDeYi", "\\192.168.10.250\public\renjianzuideyi", "8幢_人间最得意"),
    @("WuDiJianHun", "\\192.168.10.250\public\WuDiJianHun", "8幢_无敌剑魂"),
    @("JianJi", "\\192.168.10.250\public\11-剪辑", "8幢_剪辑"),
    @("HouQi", "\\192.168.10.250\public\HouQi", "8幢_后期"),
    @("MeiYiGiao", "\\192.168.10.250\public\美易高", "美易高"),
    @("LianQiShiWanNian", "\\192.168.10.250\public\LianQiShiWanNian", "8幢_炼气十万年"),
    @("WuJinShenYu", "\\192.168.10.250\public\WuJinShenYu", "8幢_无尽神域"),

    @("WanYuFengShen", "\\192.168.10.218\WanYuFengShen", "9幢_万域封神"),
    @("KuangShenMoZun", "\\192.168.10.218\KuangShenMoZun", "9幢_狂神魔尊"),
    @("JianJi", "\\192.168.10.218\jianji", "9幢_剪辑"),
    @("HouQi", "\\192.168.10.218\houqi", "9幢_后期")
    
)
# Get-ItemProperty "D:\Autodesk\test\Gumu.ma"

# $fso = New-Object -ComObject Scripting.FileSystemObject
# $folder = $fso.GetFolder("D:\Autodesk\test") Variant

# $app_shell  = New-Object -ComObject Shell.Application
# $folder = $app_shell.NameSpace("D:\Autodesk\test")
# $folder.SetDetailsOf("test2.ma",24)


# $folder = (New-Object -ComObject Shell.Application).NameSpace("$pwd")
# # Note: Assumes that no indices higher than 1000 exist.
# 0..1000 | % { 
#   if ($n = $folder.GetDetailsOf($null, $_)) { 
#     [pscustomobject] @{ Index = $_; Name = $n } 
#   } 
# }

foreach ($item in $map_item) {
    try {
        Get-Childitem -Path $item[1] -ErrorAction Stop
        if (Test-Path $item[1]) {
            if (Test-Path $item[0]) {
                $fod = Get-Item -Path $item[0]
                $fod.Delete()
            }
            New-Item -ItemType SymbolicLink -Path $PWD/$item[0] -Target $item[1]
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