set my_pwd=%~dp0
echo %my_pwd%

echo -----------------set env--------------------
call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat"

cd %my_pwd%

echo -----------------config main exe--------------------
"C:\Program Files\CMake\bin\cmake.exe" ^
-S%my_pwd% ^
--preset Ninja_release

echo -----------------build main exe--------------------
"C:\Program Files\CMake\bin\cmake.exe" ^
--build ^
--preset release_exe

echo -----------------install main exe--------------------
"C:\Program Files\CMake\bin\cmake.exe" ^
--install %my_pwd%\build\Ninja_release ^
--component exe_com

"C:\Program Files\CMake\bin\cmake.exe" ^
--install %my_pwd%\build\Ninja_release ^
--component ue4_plug

"C:\Program Files\CMake\bin\cmake.exe" ^
--install %my_pwd%\build\Ninja_release ^
--component doodle_houdini_com


@REM echo -----------------clear main--------------------
@REM del  %my_pwd%build\Ninja_release\CMakeCache.txt

@REM echo -----------------------------------------------------
@REM echo -----------------config maya 2018--------------------
@REM "C:\Program Files\CMake\bin\cmake.exe" ^
@REM -S%my_pwd% ^
@REM --preset Ninja_release -DMaya_Version=2018
@REM echo -----------------build maya 2018--------------------
@REM "C:\Program Files\CMake\bin\cmake.exe" ^
@REM --build ^
@REM --preset release_exe --target doodle_maya
@REM echo -----------------install maya 2018--------------------
@REM "C:\Program Files\CMake\bin\cmake.exe" ^
@REM --install %my_pwd%\build\Ninja_release ^
@REM --component maya_plug_com
@REM echo -----------------clear maya 2018--------------------
@REM rmdir /q /s %my_pwd%build\Ninja_release\src\maya_plug
@REM rmdir /q /s %my_pwd%build\Ninja_release\plug
@REM del  %my_pwd%build\Ninja_release\CMakeCache.txt

@REM echo -----------------------------------------------------
@REM echo -----------------config maya 2019--------------------
@REM "C:\Program Files\CMake\bin\cmake.exe" ^
@REM -S%my_pwd% ^
@REM --preset Ninja_release -DMaya_Version=2019
@REM echo -----------------build maya 2019--------------------
@REM "C:\Program Files\CMake\bin\cmake.exe" ^
@REM --build ^
@REM --preset release_exe --target doodle_maya
@REM echo -----------------install maya 2019--------------------
@REM "C:\Program Files\CMake\bin\cmake.exe" ^
@REM --install %my_pwd%\build\Ninja_release ^
@REM --component maya_plug_com
@REM echo -----------------clear maya 2019--------------------
@REM rmdir /q /s %my_pwd%build\Ninja_release\src\maya_plug
@REM rmdir /q /s %my_pwd%build\Ninja_release\plug
@REM del  %my_pwd%build\Ninja_release\CMakeCache.txt

@REM echo -----------------------------------------------------
@REM echo -----------------config maya 2020--------------------
@REM "C:\Program Files\CMake\bin\cmake.exe" ^
@REM -S%my_pwd% ^
@REM --preset Ninja_release -DMaya_Version=2020
@REM echo -----------------build maya 2020--------------------
@REM "C:\Program Files\CMake\bin\cmake.exe" ^
@REM --build ^
@REM --preset release_exe --target doodle_maya
@REM echo -----------------install maya 2020--------------------
@REM "C:\Program Files\CMake\bin\cmake.exe" ^
@REM --install %my_pwd%\build\Ninja_release ^
@REM --component maya_plug_com
@REM echo -----------------clear maya 2020--------------------
@REM rmdir /q /s %my_pwd%build\Ninja_release\src\maya_plug
@REM rmdir /q /s %my_pwd%build\Ninja_release\plug
@REM del  %my_pwd%build\Ninja_release\CMakeCache.txt

echo -----------------config pack--------------------
"C:\Program Files\CMake\bin\cmake.exe" ^
-S%my_pwd% ^
--preset Ninja_release
echo -----------------pack---------------------
"C:\Program Files\CMake\bin\cmake.exe" ^
--build ^
--preset cmake_pack

@REM echo -----------------copy file--------------------
@REM robocopy %my_pwd%build\Ninja_release\html %my_pwd%build\html /s /NFL /NDL
@REM mkdir %my_pwd%build\html\file
@REM robocopy %my_pwd%build\install %my_pwd%build\html\file *.msi
@REM robocopy %my_pwd%build\install %my_pwd%build\html\file *.7z
@REM py %my_pwd%doc\generate_directory_index_caddystyle.py %my_pwd%\build\html\file

@REM rmdir /q /s %my_pwd%build\install\bin
@REM rmdir /q /s %my_pwd%build\install\maya
@REM rmdir /q /s %my_pwd%build\install\ue425_Plug
@REM rmdir /q /s %my_pwd%build\install\ue426_Plug
@REM rmdir /q /s %my_pwd%build\install\ue427_Plug

Exit 0
