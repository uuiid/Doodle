REM Set-NetFirewallRule -DisplayGroup “File And Printer Sharing” -Enabled True -Profile Private
cd %~dp0

set current_branch=
for /F "delims=" %%n in ('git describe --tags --match "v*"') do set "current_branch=%%n"

for /f "tokens=1 delims=-" %%i in ("%current_branch%") do (set current_branch=%%i)
set current_branch=%current_branch:~1%
set Doodle_Name=Doodle-%current_branch%-win64
PsExec.exe \\192.168.20.2 -u auto_light -p root -i cmd.exe /C "\\192.168.20.59\doodle2\build_script\deploy\detail\server_cmd.cmd %Doodle_Name%"

