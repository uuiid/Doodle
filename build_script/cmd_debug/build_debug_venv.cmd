call %~dp0/../build/set_venv.cmd


echo -----------------config main exe--------------------
"C:\Program Files\CMake\bin\cmake.exe" ^
-S%my_pwd% ^
--preset Ninja_debug%Doodle_suffix%

if %errorlevel% NEQ 0 exit 1

echo -----------------build main exe--------------------
"C:\Program Files\CMake\bin\cmake.exe" ^
--build ^
--preset Ninja_debug%Doodle_suffix% ^
--target %*