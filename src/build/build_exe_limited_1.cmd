call %~dp0/set_venv.cmd limited_1

echo -----------------config main exe--------------------
"C:\Program Files\CMake\bin\cmake.exe" ^
-S%my_pwd% ^
--preset Ninja_release%Doodle_suffix% -DBUILD_LIMITED_1=ON

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

robocopy %my_pwd%\build\install\%doodle_install_prefix% \\192.168.10.250\public\Prism_projects\doodle\%doodle_install_prefix%\ *.msi 
robocopy %my_pwd%\build\install\%doodle_install_prefix% \\192.168.10.250\public\Prism_projects\doodle\%doodle_install_prefix%\ *.7z  

set errorlevel =0