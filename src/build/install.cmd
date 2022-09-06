call ./set_venv.cmd

echo -----------------config pack--------------------
"C:\Program Files\CMake\bin\cmake.exe" ^
-S%my_pwd% ^
--preset Ninja_release

if %errorlevel% NEQ 0 exit 1
echo -----------------pack---------------------
"C:\Program Files\CMake\bin\cmake.exe" ^
--build ^
--preset cmake_pack

if %errorlevel% NEQ 0 exit 1

echo -----------------copy file--------------------
mkdir %my_pwd%\build\html\file
robocopy %my_pwd%\build\install %my_pwd%\build\html\file *.msi
robocopy %my_pwd%\build\install %my_pwd%\build\html\file *.7z


