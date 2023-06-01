call %~dp0/set_venv.cmd

echo -----------------config main exe--------------------
"C:\Program Files\CMake\bin\cmake.exe" ^
-S%my_pwd% ^
--preset Ninja_release%Doodle_suffix%

if %errorlevel% NEQ 0 exit 1

echo -----------------build doxygen--------------------
"C:\Program Files\CMake\bin\cmake.exe" ^
--build ^
--preset doxygen

echo -----------------copy file--------------------
robocopy build\Ninja_release%Doodle_suffix%\src\html build\html /s /NFL /NDL

py %my_pwd%/docs/generate_directory_index_caddystyle.py %my_pwd%/build/html/file
py %my_pwd%/docs/generate_updata_log.py %my_pwd%/build/html/update.html
