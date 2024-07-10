call %~dp0/../tool/get_build_name.cmd
cd /D %~dp0

PsExec.exe @work_coms.txt  -u auto_light -i cmd /C "\\192.168.40.53\doodle2\build_script\deploy\detail\copy_UE.cmd"
