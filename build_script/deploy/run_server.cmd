REM Set-NetFirewallRule -DisplayGroup “File And Printer Sharing” -Enabled True -Profile Private
call %~dp0/../tool/get_build_name.cmd
cd /D %~dp0
PsExec.exe \\192.168.20.2 -u auto_light -p root -i cmd.exe /C "\\192.168.20.59\doodle2\build_script\deploy\detail\server_cmd.cmd %Doodle_Name%"

