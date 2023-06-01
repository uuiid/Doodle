call %~dp0/../build/set_venv.cmd


echo -----------------config main exe--------------------
"C:\Program Files\CMake\bin\cmake.exe" ^
-S%my_pwd% ^
--preset vs2022
