set(CPACK_NSIS_EXTRA_INSTALL_COMMANDS
        [==[
        CreateShortCut "$DESKTOP\Doodle ${VERSION}.lnk" "$INSTDIR\bin\DoodleExe.exe"

        WriteRegStr HKCU "SOFTWARE\Classes\doodle.main" "" "doodle"
        WriteRegStr HKCU "SOFTWARE\Classes\doodle.main\DefaultIcon" "" "$INSTDIR\bin\DoodleExe.exe"
        WriteRegStr HKCU "SOFTWARE\Classes\doodle.main\shell\open\command" "" '"$INSTDIR\bin\DoodleExe.exe" "%1"'
        WriteRegStr HKCU "SOFTWARE\Classes\.doodle_db" "" "doodle.main"
        WriteRegStr HKCU "SOFTWARE\Classes\.doodle_db\OpenWithProgids" "doodle.main" ""
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
        ]==]

)