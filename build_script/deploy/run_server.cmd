REM Set-NetFirewallRule -DisplayGroup “File And Printer Sharing” -Enabled True -Profile Private
call %~dp0/../tool/get_build_name.cmd
cd /D %~dp0
PsExec.exe \\192.168.40.181 -u auto_light -p root -i cmd.exe /C "\\192.168.20.89\doodle2\build_script\deploy\detail\kitsu.cmd %Doodle_Name%"

