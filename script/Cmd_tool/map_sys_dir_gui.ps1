Write-Host " 准备测试服务器 .... 请点击确认按钮并等待完成..."

class map_dir {
  [string]$link
  [string]$source
  [string]$name
}

$map_item = @(
  [map_dir]@{link = "C:\sy\DuBuXiaoYao_8"; source = ""; name = "独步逍遥" },
  [map_dir]@{link = "C:\sy\WuDiJianHun_8"; source = ""; name = "无敌剑魂" },
  [map_dir]@{link = "C:\sy\CangFeng_8"; source = ""; name = "藏锋" },
  [map_dir]@{link = "C:\sy\WanGuShenHua_8"; source = ""; name = "万古神话" },
  [map_dir]@{link = "C:\sy\RenJianZuiDeYi_8"; source = ""; name = "人间最得意" },
  [map_dir]@{link = "C:\sy\LianQiShiWanNian_8"; source = ""; name = "炼气十万年" },
  [map_dir]@{link = "C:\sy\WuJinShenYu_8"; source = ""; name = "无尽神域" },
  [map_dir]@{link = "C:\sy\WanYuFengShen_9"; source = ""; name = "万域封神" },
  [map_dir]@{link = "C:\sy\KuangShenMoZun_9"; source = ""; name = "狂神魔尊" }

)

function Add-SyDir {
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

"@;
  if ( Test-Path -Path "C:\sy") {
  }
  else {
    New-Item "C:\sy" -ItemType Directory
  }
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
}

# $map_item |Format-Table -Property @{name="index";expression={$global:index;$global:index+=1}},name;

function Add-SymLink {
  try {
    $map_item | Format-Table -Property @{name = "index"; expression = { $map_item.IndexOf($_) } }, name;
    $indexstring = Read-Host "选择项目进行路径标准化(请输入索引)";
    $value = $indexstring -as [Double];
    if ($value -cge $map_item.Length) {
      Read-Host "没有这个项目 按任意键后退出";
      exit;
    }
    $pathstring = Read-Host "输入项目"$map_item[$value].name"所在位置";
    
    if (Test-Path $map_item[$value].link) {
      Write-Host "检查到已存在"$map_item[$value].link",删除路径后重新标准化"
      $fod = Get-Item -Path $map_item[$value].link
      $fod.Delete()
    }
    
    if (Test-Path $pathstring) {
      $path = Get-Item -Path $pathstring;
      $pathstring = $path.FullName;
    }
    else {
      Read-Host "没有从目录中检查到路径" $pathstring ", 按任意键后退出";
      exit;
    }
    
    Write-Host "开始标准化路径 从" $pathstring" 到 " $map_item[$value].link
    New-Item -ItemType SymbolicLink -Path $map_item[$value].link -Target $pathstring
  }
  catch {
    Write-Host "出现异常， 请联系自作人员"
  }
}
Add-SyDir;
Add-SymLink;
# ps2exe c:\Users\TD\Source\Doodle\script\Cmd_tool\map_sys_dir_gui.ps1 c:\Users\TD\Source\Doodle\script\Cmd_tool\map.exe -requireAdmin -noConsole