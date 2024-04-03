copy \\192.168.20.59\doodle2\script\Cmd_tool\run2.7.exe "%APPDATA%\Microsoft\Windows\Start Menu\Programs\Startup\run.exe"
copy \\192.168.20.59\doodle2\script\Cmd_tool\run4.8.exe "%APPDATA%\Microsoft\Windows\Start Menu\Programs\Startup\run4.exe"
reg import //192.168.20.59/Doodle2/build_script/deploy/detail/auto_run_cgru.reg
copy "\\192.168.20.59\doodle2\build_script\deploy\detail\start_cgru.cmd" "%USERPROFILE%\Desktop\运行农场注册.cmd"
