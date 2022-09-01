set my_pwd=%~dp0
echo %my_pwd%

echo -----------------set env--------------------
call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat"

cd %my_pwd%

echo -----------------config main exe--------------------
"C:\Program Files\CMake\bin\cmake.exe" ^
-S%my_pwd% ^
--preset Ninja_release

if %errorlevel% NEQ 0 exit 1

echo -----------------build main exe--------------------
"C:\Program Files\CMake\bin\cmake.exe" ^
--build ^
--preset release_exe


if %errorlevel% NEQ 0 exit 1
echo -----------------install main exe--------------------
"C:\Program Files\CMake\bin\cmake.exe" ^
--install %my_pwd%\build\Ninja_release ^
--component exe_com


if %errorlevel% NEQ 0 exit 1
"C:\Program Files\CMake\bin\cmake.exe" ^
--install %my_pwd%\build\Ninja_release ^
--component ue4_plug


if %errorlevel% NEQ 0 exit 1
"C:\Program Files\CMake\bin\cmake.exe" ^
--install %my_pwd%\build\Ninja_release ^
--component doodle_houdini_com


if %errorlevel% NEQ 0 exit 1

echo -----------------clear main--------------------
del  %my_pwd%build\Ninja_release\CMakeCache.txt

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
rmdir /q /s %my_pwd%build\Ninja_release\src\maya_plug
rmdir /q /s %my_pwd%build\Ninja_release\plug
del  %my_pwd%build\Ninja_release\CMakeCache.txt

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
rmdir /q /s %my_pwd%build\Ninja_release\src\maya_plug
rmdir /q /s %my_pwd%build\Ninja_release\plug
del  %my_pwd%build\Ninja_release\CMakeCache.txt

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
rmdir /q /s %my_pwd%build\Ninja_release\src\maya_plug
rmdir /q /s %my_pwd%build\Ninja_release\plug
del  %my_pwd%build\Ninja_release\CMakeCache.txt

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
robocopy %my_pwd%build\Ninja_release\html %my_pwd%build\html /s /NFL /NDL
mkdir %my_pwd%build\html\file
robocopy %my_pwd%build\install %my_pwd%build\html\file *.msi
robocopy %my_pwd%build\install %my_pwd%build\html\file *.7z
call ./docs/doc.cmd

rmdir /q /s %my_pwd%build\install\bin
rmdir /q /s %my_pwd%build\install\maya
rmdir /q /s %my_pwd%build\install\ue425_Plug
rmdir /q /s %my_pwd%build\install\ue426_Plug
rmdir /q /s %my_pwd%build\install\ue427_Plug
rmdir /q /s %my_pwd%build\install\houdini
rmdir /q /s %my_pwd%build\install\SideFX_Labs

Exit 0
