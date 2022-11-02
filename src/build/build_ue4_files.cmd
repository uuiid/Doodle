call %~dp0/set_venv.cmd


echo -----------------config ue4--------------------
"C:\Program Files\CMake\bin\cmake.exe" ^
-S%my_pwd% ^
--preset Ninja_release%Doodle_suffix%

if %errorlevel% NEQ 0 exit 1



echo -----------------install ue4--------------------
"C:\Program Files\CMake\bin\cmake.exe" ^
--install %my_pwd%\build\Ninja_release%Doodle_suffix% ^
--component ue4_plug


if %errorlevel% NEQ 0 exit 1
