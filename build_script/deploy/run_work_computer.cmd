call %~dp0/../tool/get_build_name.cmd
cd /D %~dp0
@REM PsExec.exe \\192.168.20.7 -u administrator -p root -i cmd.exe /C "\\192.168.20.89\doodle2\build_script\deploy\detail\work_computer_cmd.cmd %Doodle_Name%"
@REM PsExec.exe \\192.168.20.66  -u auto_light         -i cmd.exe /C "\\192.168.20.89\doodle2\build_script\deploy\detail\work_computer_cmd.cmd %Doodle_Name%"
@REM PsExec.exe \\192.168.20.65  -u auto_light         -i cmd.exe /C "\\192.168.20.89\doodle2\build_script\deploy\detail\work_computer_cmd.cmd %Doodle_Name%"
@REM PsExec.exe \\192.168.20.190 -u auto_light -p root -i cmd.exe /C "\\192.168.20.89\doodle2\build_script\deploy\detail\work_computer_cmd.cmd %Doodle_Name%"

@REM PsExec.exe \\192.168.20.188 -u auto_light  -i cmd.exe /C "\\192.168.20.89\doodle2\build_script\deploy\detail\work_computer_cmd.cmd %Doodle_Name%"
@REM PsExec.exe @work_coms.txt  -u auto_light -i cmd /C "\\192.168.20.89\doodle2\build_script\deploy\detail\run_render.cmd"
@REM PsExec.exe @work_coms.txt  -u auto_light -i cmd /C "\\192.168.20.89\doodle2\build_script\deploy\detail\copy_run.cmd"
PsExec.exe @work_coms.txt  -u auto_light -i cmd /C "\\192.168.20.89\doodle2\build_script\deploy\detail\work_computer_cmd.cmd %Doodle_Name%"

@REM PsShutdown.exe @work_coms.txt  -u auto_light -r