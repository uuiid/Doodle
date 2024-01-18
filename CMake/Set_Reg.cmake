set(CPACK_NSIS_EXTRA_INSTALL_COMMANDS
        [==[
        CreateShortCut "$DESKTOP\Doodle ${VERSION}.lnk" "$INSTDIR\bin\DoodleExe.exe"
        WriteRegStr HKCU "SOFTWARE\Doodle\RenderFarm" "server_address" "192.168.20.7"


        WriteRegStr HKCU "SOFTWARE\Classes\doodle.main" "" "doodle"
        WriteRegStr HKCU "SOFTWARE\Classes\doodle.main\DefaultIcon" "" "$INSTDIR\bin\DoodleExe.exe"
        WriteRegStr HKCU "SOFTWARE\Classes\doodle.main\shell\open\command" "" '"$INSTDIR\bin\DoodleExe.exe" "%1"'
        WriteRegStr HKCU "SOFTWARE\Classes\.doodle_db" "" "doodle.main"
        WriteRegStr HKCU "SOFTWARE\Classes\.doodle_db\OpenWithProgids" "doodle.main" ""


        SetRegView 64
        WriteRegStr HKLM "SOFTWARE\Doodle\MainConfig" "main_project" "//192.168.10.218/Doodletemp/db_file/doodle_main.doodle_db"
        WriteRegStr HKLM "SOFTWARE\Doodle\MainConfig" "update_path" "//192.168.10.218/Doodletemp/auto_light"
        WriteRegStr HKLM "SOFTWARE\Doodle\MainConfig" "install_dir" "$INSTDIR\bin"

        WriteRegStr HKLM "HKEY_LOCAL_MACHINE\SOFTWARE\Doodle\MainConfig\ProjectList\DB" "" "独步逍遥"
        WriteRegStr HKLM "HKEY_LOCAL_MACHINE\SOFTWARE\Doodle\MainConfig\ProjectList\DB" "path" "//192.168.10.250/public/DuBuXiaoYao_3"
        WriteRegStr HKLM "HKEY_LOCAL_MACHINE\SOFTWARE\Doodle\MainConfig\ProjectList\DB" "en_str" "DuBuXiaoYao"

        WriteRegStr HKLM "HKEY_LOCAL_MACHINE\SOFTWARE\Doodle\MainConfig\ProjectList\DW" "" "万古邪帝"
        WriteRegStr HKLM "HKEY_LOCAL_MACHINE\SOFTWARE\Doodle\MainConfig\ProjectList\DW" "path" "//192.168.10.240/public/WGXD"
        WriteRegStr HKLM "HKEY_LOCAL_MACHINE\SOFTWARE\Doodle\MainConfig\ProjectList\DW" "en_str" "WanGuXieDi"

        WriteRegStr HKLM "HKEY_LOCAL_MACHINE\SOFTWARE\Doodle\MainConfig\ProjectList\LM" "" "龙脉武神"
        WriteRegStr HKLM "HKEY_LOCAL_MACHINE\SOFTWARE\Doodle\MainConfig\ProjectList\LM" "path" "//192.168.10.240/public/LongMaiWuShen"
        WriteRegStr HKLM "HKEY_LOCAL_MACHINE\SOFTWARE\Doodle\MainConfig\ProjectList\LM" "en_str" "LongMaiWuShen"

        WriteRegStr HKLM "HKEY_LOCAL_MACHINE\SOFTWARE\Doodle\MainConfig\ProjectList\LQ" "" "炼气十万年"
        WriteRegStr HKLM "HKEY_LOCAL_MACHINE\SOFTWARE\Doodle\MainConfig\ProjectList\LQ" "path" "//192.168.10.240/public/LianQiShiWanNian"
        WriteRegStr HKLM "HKEY_LOCAL_MACHINE\SOFTWARE\Doodle\MainConfig\ProjectList\LQ" "en_str" "LianQiShiWanNian"

        WriteRegStr HKLM "HKEY_LOCAL_MACHINE\SOFTWARE\Doodle\MainConfig\ProjectList\RJ" "" "人间最得意"
        WriteRegStr HKLM "HKEY_LOCAL_MACHINE\SOFTWARE\Doodle\MainConfig\ProjectList\RJ" "path" "//192.168.10.240/public/renjianzuideyi"
        WriteRegStr HKLM "HKEY_LOCAL_MACHINE\SOFTWARE\Doodle\MainConfig\ProjectList\RJ" "en_str" "RenJianZuiDeYi"

        WriteRegStr HKLM "HKEY_LOCAL_MACHINE\SOFTWARE\Doodle\MainConfig\ProjectList\WD" "" "无敌剑魂"
        WriteRegStr HKLM "HKEY_LOCAL_MACHINE\SOFTWARE\Doodle\MainConfig\ProjectList\WD" "path" "//192.168.10.240/public/WuDiJianHun"
        WriteRegStr HKLM "HKEY_LOCAL_MACHINE\SOFTWARE\Doodle\MainConfig\ProjectList\WD" "en_str" "WuDiJianHun"

        WriteRegStr HKLM "HKEY_LOCAL_MACHINE\SOFTWARE\Doodle\MainConfig\ProjectList\WG" "" "万古神话"
        WriteRegStr HKLM "HKEY_LOCAL_MACHINE\SOFTWARE\Doodle\MainConfig\ProjectList\WG" "path" "//192.168.10.240/public/WanGuShenHua"
        WriteRegStr HKLM "HKEY_LOCAL_MACHINE\SOFTWARE\Doodle\MainConfig\ProjectList\WG" "en_str" "WanGuShenHua"

        WriteRegStr HKLM "HKEY_LOCAL_MACHINE\SOFTWARE\Doodle\MainConfig\ProjectList\WJ" "" "无尽神域"
        WriteRegStr HKLM "HKEY_LOCAL_MACHINE\SOFTWARE\Doodle\MainConfig\ProjectList\WJ" "path" "//192.168.10.240/public/WuJinShenYu"
        WriteRegStr HKLM "HKEY_LOCAL_MACHINE\SOFTWARE\Doodle\MainConfig\ProjectList\WJ" "en_str" "WuJinShenYu"

        WriteRegStr HKLM "HKEY_LOCAL_MACHINE\SOFTWARE\Doodle\MainConfig\ProjectList\WY" "" "万域封神"
        WriteRegStr HKLM "HKEY_LOCAL_MACHINE\SOFTWARE\Doodle\MainConfig\ProjectList\WY" "path" "//192.168.10.218/WanYuFengShen"
        WriteRegStr HKLM "HKEY_LOCAL_MACHINE\SOFTWARE\Doodle\MainConfig\ProjectList\WY" "en_str" "WanYuFengShen"

        SetRegView 32


       ]==]
)
set(CPACK_NSIS_EXTRA_UNINSTALL_COMMANDS
        [==[
        Delete "$DESKTOP\Doodle ${VERSION}.lnk"
        DeleteRegKey HKCU "SOFTWARE\Classes\.doodle_db\OpenWithProgids"
        DeleteRegKey HKCU "SOFTWARE\Classes\.doodle_db"
        DeleteRegKey HKCU "SOFTWARE\Classes\doodle.main\shell\open\command"
        DeleteRegKey HKCU "SOFTWARE\Classes\doodle.main\DefaultIcon"
        DeleteRegKey HKCU "SOFTWARE\Classes\doodle.main"

        SetRegView 64
        DeleteRegKey HKLM "SOFTWARE\Doodle\RenderFarm"
        DeleteRegKey HKLM "SOFTWARE\Doodle\MainConfig"
        DeleteRegKey HKLM "SOFTWARE\Doodle\MainConfig\ProjectList"
        SetRegView 32
        ]==]

)