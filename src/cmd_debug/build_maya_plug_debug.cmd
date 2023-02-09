call %~dp0/../build/set_venv.cmd

echo -----------------------------------------------------
echo -----------------config maya 2018--------------------
"C:\Program Files\CMake\bin\cmake.exe" ^
-S%my_pwd% ^
--preset Ninja_RelWithDebInfo

if %errorlevel% NEQ 0 exit 1
echo -----------------build maya 2018--------------------
"C:\Program Files\CMake\bin\cmake.exe" ^
--build ^
--preset debug_maya_plug --target doodle_maya

robocopy %my_pwd%\build\doodle_maya\bin %my_pwd%\build\doodle_maya\plug\maya\plug-ins *.dll > %tmp%/build_maya_plug_debug.txt

exit 0