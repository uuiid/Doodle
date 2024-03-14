REM Set-NetFirewallRule -DisplayGroup “File And Printer Sharing” -Enabled True -Profile Private
cd %~dp0
PsExec.exe \\192.168.20.2 -u auto_light -p root -i cmd \\192.168.20.59\doodle2\build_script\deploy\server_cmd.cmd
PsExec.exe \\192.168.20.2 -u auto_light -p root -i cmd.exe "/C \\192.168.20.59\doodle2\build_script\deploy\server_cmd.cmd"
PsExec.exe \\192.168.20.2 -u auto_light -p root -i cmd.exe /C "\\192.168.20.59\doodle2\build_script\deploy\server_cmd.cmd"

