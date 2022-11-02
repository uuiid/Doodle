call %~dp0/set_venv.cmd

echo -----------------config pack--------------------
"C:\Program Files\CMake\bin\cmake.exe" ^
-S%my_pwd% ^
--preset Ninja_release%Doodle_suffix%

if %errorlevel% NEQ 0 exit 1
echo -----------------pack---------------------
"C:\Program Files\CMake\bin\cmake.exe" ^
--build ^
--preset cmake_pack%Doodle_suffix%

if %errorlevel% NEQ 0 exit 1

echo -----------------copy file--------------------
if not exist "%my_pwd%\build\html\file" goto create_file_dir
goto copy_file

:create_file_dir
mkdir %my_pwd%\build\html\file

:copy_file
echo "%my_pwd%\build\html\file *.msi -> %my_pwd%\build\install"
robocopy %my_pwd%\build\install %my_pwd%\build\html\file *.msi > %tmp%/doodle_install_copy1.txt

echo "%my_pwd%\build\html\file *.7z -> %my_pwd%\build\install "
robocopy %my_pwd%\build\install %my_pwd%\build\html\file *.7z > %tmp%/doodle_install_copy2.txt

echo "generate %my_pwd%/build/html/file/index.html"
py %my_pwd%/docs/generate_directory_index_caddystyle.py %my_pwd%/build/html/file

echo "generate %my_pwd%/build/html/update.html"
py %my_pwd%/docs/generate_updata_log.py %my_pwd%/build/html/update.html
