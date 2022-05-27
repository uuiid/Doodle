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

echo -----------------------------------------------------
echo -----------------config maya 2018--------------------
"C:\Program Files\CMake\bin\cmake.exe" ^
-S%my_pwd% ^
--preset Ninja_release_plug -DMaya_Version=2018
echo -----------------build maya 2018--------------------
"C:\Program Files\CMake\bin\cmake.exe" ^
--build ^
--preset release_maya_plug
echo -----------------install maya 2018--------------------
"C:\Program Files\CMake\bin\cmake.exe" ^
--install %my_pwd%\build\Ninja_release_plug ^
--component maya_plug_com
echo -----------------clear maya 2018--------------------
rmdir /q /s %my_pwd%build\Ninja_release_plug\src\maya_plug
rmdir /q /s %my_pwd%build\Ninja_release_plug\plug
del  %my_pwd%build\Ninja_release_plug\CMakeCache.txt

echo -----------------------------------------------------
echo -----------------config maya 2019--------------------
"C:\Program Files\CMake\bin\cmake.exe" ^
-S%my_pwd% ^
--preset Ninja_release_plug -DMaya_Version=2019
echo -----------------build maya 2019--------------------
"C:\Program Files\CMake\bin\cmake.exe" ^
--build ^
--preset release_maya_plug
echo -----------------install maya 2019--------------------
"C:\Program Files\CMake\bin\cmake.exe" ^
--install %my_pwd%\build\Ninja_release_plug ^
--component maya_plug_com
echo -----------------clear maya 2019--------------------
rmdir /q /s %my_pwd%build\Ninja_release_plug\src\maya_plug
rmdir /q /s %my_pwd%build\Ninja_release_plug\plug
del  %my_pwd%build\Ninja_release_plug\CMakeCache.txt

echo -----------------------------------------------------
echo -----------------config maya 2020--------------------
"C:\Program Files\CMake\bin\cmake.exe" ^
-S%my_pwd% ^
--preset Ninja_release_plug -DMaya_Version=2020
echo -----------------build maya 2020--------------------
"C:\Program Files\CMake\bin\cmake.exe" ^
--build ^
--preset release_maya_plug
echo -----------------install maya 2020--------------------
"C:\Program Files\CMake\bin\cmake.exe" ^
--install %my_pwd%\build\Ninja_release_plug ^
--component maya_plug_com
echo -----------------clear maya 2020--------------------
rmdir /q /s %my_pwd%build\Ninja_release_plug\src\maya_plug
rmdir /q /s %my_pwd%build\Ninja_release_plug\plug
del  %my_pwd%build\Ninja_release_plug\CMakeCache.txt



echo -----------------pack---------------------
"C:\Program Files\CMake\bin\cmake.exe" ^
--build ^
--preset cmake_pack

echo -----------------copy file--------------------
robocopy %my_pwd%build\Ninja_release\html %my_pwd%build\html /s /NFL /NDL
mkdir %my_pwd%build\html\file
robocopy %my_pwd%build\install %my_pwd%build\html\file *.msi
robocopy %my_pwd%build\install %my_pwd%build\html\file *.7z
py %my_pwd%doc\generate_directory_index_caddystyle.py %my_pwd%\build\html\file

rmdir /q /s %my_pwd%build\install\bin
rmdir /q /s %my_pwd%build\install\maya
rmdir /q /s %my_pwd%build\install\ue425_Plug
rmdir /q /s %my_pwd%build\install\ue426_Plug
rmdir /q /s %my_pwd%build\install\ue427_Plug

Exit 0
