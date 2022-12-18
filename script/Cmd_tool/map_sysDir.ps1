Write-Host " 准备测试服务器 .... 请点击确认按钮并等待完成..."
class map_dir {
    [string]$link;
    [string]$source;
    [string]$name;
}

$map_item = @(
    [map_dir]@{link = "C:\sy\DuBuXiaoYao_8"; source = "\\192.168.10.250\public\DuBuXiaoYao_3"; name = "独步逍遥v3_250" },
    [map_dir]@{link = "C:\sy\ChengXv_8"; source = "\\192.168.10.250\public\Prism_projects"; name = "程序开发_250" },
    [map_dir]@{link = "C:\sy\donghuagongxiang_8"; source = "\\192.168.10.250\public\动画共享"; name = "动画共享_250" },
    [map_dir]@{link = "C:\sy\RenJianZuiDeYi_8"; source = "\\192.168.10.250\public\renjianzuideyi"; name = "人间最得意_250" },
    [map_dir]@{link = "C:\sy\HouQi_8"; source = "\\192.168.10.250\public\HouQi"; name = "后期_250" },
    [map_dir]@{link = "C:\sy\MeiYiGiao_8"; source = "\\192.168.10.250\public\美易高"; name = "美易高" },
    [map_dir]@{link = "C:\sy\jianji_250"; source = "\\192.168.10.250\public\11-剪辑" ; name = "剪辑_250" },
    
    [map_dir]@{link = "C:\sy\ChangAnHuanJie_8"; source = "\\192.168.10.240\public\changanhuanjie"; name = "长安幻街_240" },
    [map_dir]@{link = "C:\sy\WuJinShenYu_8"; source = "\\192.168.10.240\public\WuJinShenYu"; name = "无尽神域_240" },
    [map_dir]@{link = "C:\sy\WuDiJianHun_8"; source = "\\192.168.10.240\public\WuDiJianHun"; name = "无敌剑魂_240" },
    [map_dir]@{link = "C:\sy\WanGuShenHua_8"; source = "\\192.168.10.240\public\WanGuShenHua"; name = "万古神话_240" },
    [map_dir]@{link = "C:\sy\LianQiShiWanNian_8"; source = "\\192.168.10.240\public\LianQiShiWanNian"; name = "炼气十万年_240" },
    [map_dir]@{link = "C:\sy\CangFeng_8"; source = "\\192.168.10.240\public\CangFeng"; name = "藏锋_240" },
    [map_dir]@{link = "C:\sy\JianJi_8"; source = "\\192.168.10.240\public\剪辑_240"; name = "剪辑_240" },
    [map_dir]@{link = "C:\sy\WGXD"; source = "\\192.168.10.240\public\WGXD"; name = "万古邪帝_240" },
    [map_dir]@{link = "C:\sy\LongMaiWuShen"; source = "\\192.168.10.240\public\LongMaiWuShen"; name = "龙脉武神_240" },
    
    [map_dir]@{link = "C:\sy\WanYuFengShen_9"; source = "\\192.168.10.218\WanYuFengShen"; name = "万域封神_218" },
    [map_dir]@{link = "C:\sy\KuangShenMoZun_9"; source = "\\192.168.10.218\KuangShenMoZun"; name = "狂神魔尊_218" },
    [map_dir]@{link = "C:\sy\JianJi_9"; source = "\\192.168.10.218\jianji"; name = "剪辑_218" },
    [map_dir]@{link = "C:\sy\HouQi_9"; source = "\\192.168.10.218\houqi"; name = "后期_218" },
    [map_dir]@{link = "C:\sy\doodle"; source = "\\192.168.10.240\public\doodle"; name = "软件" }
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
}

function Add-SymLink {
    foreach ($item in $map_item) {
        $item -is [map_dir]
        
        try {
            Get-Childitem -Path $item.source -ErrorAction Stop
            if (Test-Path $item.source) {
                if (Test-Path $item.link) {
                    $fod = Get-Item -Path $item.link
                    $fod.Delete()
                }
                New-Item -ItemType SymbolicLink -Path $item.link -Target $item.source
            }
        }
        catch [System.Management.Automation.ActionPreferenceStopException] {
            Write-Host "目录" $item.source "没有访问权限， 取消映射"
        }
        catch {
            Write-Host "catch all 目录 " $item.source " 没有访问权限， 取消映射"
        }
    }
}

function Add-Tile {
    param (
        
    )
    foreach ($item in $map_item) {
        Write-Host "开始创建 目录 " $item.source " 的别名"
        $ini = $item.source + "\desktop.ini"
        $con = @"
[.ShellClassInfo]
InfoTip=@Shell32.dll,-12688
IconFile=%SystemRoot%\system32\mydocs.dll
IconIndex=-101
IconResource=C:\WINDOWS\System32\SHELL32.dll,43

[{F29F85E0-4FF9-1068-AB91-08002B27B3D9}]
    Prop2 = 31,$($item.name)
    Prop3 = 31,secret
    Prop4 = 31,John Doe
    Prop5 = 31,how it works
    Prop6 = 31,this is comment

"@
        try {
            Set-Content -Path $ini -Value $con -Encoding "unicode" -Force -ErrorAction Stop
            $file = Get-Item -Path $ini -Force
            $file.Attributes = 'Archive, System, Hidden'
        }
        catch [System.Management.Automation.ActionPreferenceStopException] {
            Write-Host "目录" $item.source "没有访问权限， 不写入"
        }
        catch {
            Write-Host "目录" $item.source "没有访问权限， 不写入"
        }
    }
}

Add-SyDir;
Add-SymLink;
# ps2exe F:\Doodle\script\Cmd_tool\map_sysDir.ps1 F:\Doodle\script\Cmd_tool\run4.6.exe -requireAdmin