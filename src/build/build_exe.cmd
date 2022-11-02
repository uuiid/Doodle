call %~dp0/set_venv.cmd

echo "Current Location is %my_pwd%"


echo -----------------config main exe--------------------
"C:\Program Files\CMake\bin\cmake.exe" ^
-S%my_pwd% ^
--preset Ninja_release%Doodle_suffix%

if %errorlevel% NEQ 0 exit 1

echo -----------------build main exe--------------------
"C:\Program Files\CMake\bin\cmake.exe" ^
--build ^
--preset release_exe%Doodle_suffix%


if %errorlevel% NEQ 0 exit 1
echo -----------------install main exe--------------------
"C:\Program Files\CMake\bin\cmake.exe" ^
--install %my_pwd%\build\Ninja_release%Doodle_suffix% ^
--component exe_com

call  %my_pwd%\src\build\doc.cmd > %tmp%/doodle_doc_info.txt
