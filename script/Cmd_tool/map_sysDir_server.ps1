Write-Host " 准备测试服务器 .... 请点击确认按钮并等待完成..."
# 这个是服务器使用的脚本

$map_item = @(
    @("ChangAnHuanJie", "\\192.168.10.250\public\changanhuanjie\03_Workflow", "8幢_长安幻街"),
    @("DuBuXiaoYao", "\\192.168.10.250\public\DuBuXiaoYao_3\03_Workflow", "8幢_独步逍遥v3"),
    @("ChengXv", "\\192.168.10.250\public\Prism_projects\03_Workflow", "8幢_程序开发"),
    @("donghuagongxiang", "\\192.168.10.250\public\动画共享\03_Workflow", "8幢_动画共享"),
    @("CangFeng", "\\192.168.10.250\public\CangFeng\03_Workflow", "8幢_藏锋"),
    @("WanGuShenHua", "\\192.168.10.250\public\WanGuShenHua\03_Workflow", "8幢_万古神话"),
    @("RenJianZuiDeYi", "\\192.168.10.250\public\renjianzuideyi\03_Workflow", "8幢_人间最得意"),
    @("WuDiJianHun", "\\192.168.10.250\public\WuDiJianHun\03_Workflow", "8幢_无敌剑魂"),
    @("MeiYiGiao", "\\192.168.10.250\public\美易高\03_Workflow", "美易高"),
    @("LianQiShiWanNian", "\\192.168.10.250\public\LianQiShiWanNian\03_Workflow", "8幢_炼气十万年"),
    @("WuJinShenYu", "\\192.168.10.250\public\WuJinShenYu\03_Workflow", "8幢_无尽神域"),

    @("WanYuFengShen", "\\192.168.10.218\WanYuFengShen\03_Workflow", "9幢_万域封神"),
    @("KuangShenMoZun", "\\192.168.10.218\KuangShenMoZun\03_Workflow", "9幢_狂神魔尊")
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
        $k_p = Get-Item -Path $item[0]
        if ($k_p.Exists) {
            $k_p.Delete()
        }
        Write-Host $k_p
        if (!(Test-Path -Path $item[1])) {
            New-Item -ItemType Directory -Path  $item[1]
        }
        New-Item -ItemType SymbolicLink -Path $item[0] -Target $item[1]
    }
    catch [System.Management.Automation.ActionPreferenceStopException] {
        Write-Host "目录" $item[1] "没有访问权限， 取消映射"
    }
}


# ps2exe c:\Users\TD\Source\Doodle\script\Cmd_tool\map_sysDir.ps1 c:\Users\TD\Source\Doodle\script\Cmd_tool\run4.exe -requireAdmin