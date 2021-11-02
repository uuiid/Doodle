Write-Host " 准备测试服务器 .... 请点击确认按钮并等待完成..."

class map_dir {
  [string] $link
  [string] $source
  [string] $name
}
# @("", "", "8幢_独步逍遥v3")
# @("C:\sy\ChengXv_8", "\\192.168.10.250\public\Prism_projects", "8幢_程序开发"),
# @("C:\sy\donghuagongxiang_8", "\\192.168.10.250\public\动画共享", "8幢_动画共享"),
# @("C:\sy\CangFeng_8", "\\192.168.10.250\public\CangFeng", "8幢_藏锋"),
# @("C:\sy\WanGuShenHua_8", "\\192.168.10.250\public\WanGuShenHua", "8幢_万古神话"),
# @("C:\sy\RenJianZuiDeYi_8", "\\192.168.10.250\public\renjianzuideyi", "8幢_人间最得意"),
# @("C:\sy\WuDiJianHun_8", "\\192.168.10.250\public\WuDiJianHun", "8幢_无敌剑魂"),
# @("C:\sy\JianJi_8", "\\192.168.10.250\public\11-剪辑", "8幢_剪辑"),
# @("C:\sy\HouQi_8", "\\192.168.10.250\public\HouQi", "8幢_后期"),
# @("C:\sy\MeiYiGiao_8", "\\192.168.10.250\public\美易高", "美易高"),
# @("C:\sy\LianQiShiWanNian_8", "\\192.168.10.250\public\LianQiShiWanNian", "8幢_炼气十万年"),
# @("C:\sy\WuJinShenYu_8", "\\192.168.10.250\public\WuJinShenYu", "8幢_无尽神域"),

# @("C:\sy\WanYuFengShen_9", "\\192.168.10.218\WanYuFengShen", "9幢_万域封神"),
# @("C:\sy\KuangShenMoZun_9", "\\192.168.10.218\KuangShenMoZun", "9幢_狂神魔尊"),
# @("C:\sy\JianJi_9", "\\192.168.10.218\jianji", "9幢_剪辑"),
# @("C:\sy\HouQi_9", "\\192.168.10.218\houqi", "9幢_后期")
$map_item = @(
  [map_dir]@{link = "C:\sy\WuDiJianHun_8"; source = ""; name = "无敌剑魂" }
)


if ( Test-Path -Path "C:\sy") {
}
else {
  New-Item "C:\sy" -ItemType Directory
}

$main_ini = @"
[.ShellClassInfo]
InfoTip=@Shell32.dll,-12688
IconFile=%SystemRoot%\system32\mydocs.dll
IconIndex=-101
IconResource=C:\WINDOWS\System32\SHELL32.dll,43

[{F29F85E0-4FF9-1068-AB91-08002B27B3D9}]
    Prop2 = 31,索以文化
    Prop3 = 31,secret
    Prop4 = 31,John Doe
    Prop5 = 31,how it works
    Prop6 = 31,this is comment

[ViewState]
Mode = 
Vid = {137E7700-3573-11CF-AE69-08002B2E1262}
FolderType=
Logo =

"@

Set-Content -Path "C:\sy\desktop.ini" -Value $main_ini -Encoding "unicode" -Force
$file = Get-Item -Path "C:\sy\desktop.ini" -Force
$file.Attributes = 'Archive, System, Hidden'


$log_f = $env:TMP + "/doodle_map.txt";
if (Test-Path $log_f) {
  $l_p = Get-Item -Path $log_f 
  if ($l_p.Exists) {
    $l_p.Delete()
  }
}
foreach ($item in $map_item) {
  try {
    $t_dir = Read-Host "输入"  $item.name "路径"
    Add-Content -Path $log_f -Value $t_dir -Encoding "unicode" -Force
    if ($t_dir.Length -gt 0) {
      $log = Get-Childitem -Path $t_dir -ErrorAction Stop
      Add-Content -Path $log_f -Value $log -Encoding "unicode" -Force
      if (Test-Path $t_dir) {
        if (Test-Path $item.link) {
          $fod = Get-Item -Path $item.link
          $fod.Delete()
        }
        $log = New-Item -ItemType SymbolicLink -Path $item.link -Target $t_dir
        Add-Content -Path $log_f -Value $log -Encoding "unicode" -Force
      }
    }
  }
  catch [System.Management.Automation.ActionPreferenceStopException] {
    Write-Host "目录 " $item.source "没有访问权限， 取消映射"
  }
  catch {
    Write-Host "项目 " $item.name " 未知原因， 无法创建"
  }
}
# ps2exe c:\Users\TD\Source\Doodle\script\Cmd_tool\map_sys_dir_gui.ps1 c:\Users\TD\Source\Doodle\script\Cmd_tool\map.exe -requireAdmin -noConsole