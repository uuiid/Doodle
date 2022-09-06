call ./set_venv.cmd

echo -----------------------------------------------------
echo -----------------config maya 2018--------------------
"C:\Program Files\CMake\bin\cmake.exe" ^
-S%my_pwd% ^
--preset Ninja_release -DMaya_Version=2018

if %errorlevel% NEQ 0 exit 1
echo -----------------build maya 2018--------------------
"C:\Program Files\CMake\bin\cmake.exe" ^
--build ^
--preset release_exe --target doodle_maya

if %errorlevel% NEQ 0 exit 1
echo -----------------install maya 2018--------------------
"C:\Program Files\CMake\bin\cmake.exe" ^
--install %my_pwd%\build\Ninja_release ^
--component maya_plug_com

if %errorlevel% NEQ 0 exit 1
echo -----------------clear maya 2018--------------------
rmdir /q /s %my_pwd%\build\Ninja_release\src\maya_plug
rmdir /q /s %my_pwd%\build\Ninja_release\plug
del  %my_pwd%\build\Ninja_release\CMakeCache.txt

echo -----------------------------------------------------
echo -----------------config maya 2019--------------------
"C:\Program Files\CMake\bin\cmake.exe" ^
-S%my_pwd% ^
--preset Ninja_release -DMaya_Version=2019

if %errorlevel% NEQ 0 exit 1
echo -----------------build maya 2019--------------------
"C:\Program Files\CMake\bin\cmake.exe" ^
--build ^
--preset release_exe --target doodle_maya

if %errorlevel% NEQ 0 exit 1
echo -----------------install maya 2019--------------------
"C:\Program Files\CMake\bin\cmake.exe" ^
--install %my_pwd%\build\Ninja_release ^
--component maya_plug_com

if %errorlevel% NEQ 0 exit 1
echo -----------------clear maya 2019--------------------
rmdir /q /s %my_pwd%\build\Ninja_release\src\maya_plug
rmdir /q /s %my_pwd%\build\Ninja_release\plug
del  %my_pwd%\build\Ninja_release\CMakeCache.txt

echo -----------------------------------------------------
echo -----------------config maya 2020--------------------
"C:\Program Files\CMake\bin\cmake.exe" ^
-S%my_pwd% ^
--preset Ninja_release -DMaya_Version=2020

if %errorlevel% NEQ 0 exit 1
echo -----------------build maya 2020--------------------
"C:\Program Files\CMake\bin\cmake.exe" ^
--build ^
--preset release_exe --target doodle_maya

if %errorlevel% NEQ 0 exit 1
echo -----------------install maya 2020--------------------
"C:\Program Files\CMake\bin\cmake.exe" ^
--install %my_pwd%\build\Ninja_release ^
--component maya_plug_com

if %errorlevel% NEQ 0 exit 1
echo -----------------clear maya 2020--------------------
rmdir /q /s %my_pwd%\build\Ninja_release\src\maya_plug
rmdir /q /s %my_pwd%\build\Ninja_release\plug
del  %my_pwd%\build\Ninja_release\CMakeCache.txt
