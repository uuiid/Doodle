call ./set_venv.cmd


echo -----------------config ue4--------------------
"C:\Program Files\CMake\bin\cmake.exe" ^
-S%my_pwd% ^
--preset Ninja_release

if %errorlevel% NEQ 0 exit 1



echo -----------------install ue4--------------------
"C:\Program Files\CMake\bin\cmake.exe" ^
--install %my_pwd%\build\Ninja_release ^
--component ue4_plug


if %errorlevel% NEQ 0 exit 1
