set my_pwd=%~dp0
echo %my_pwd%

echo -----------------set env--------------------
call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat"

cd %my_pwd%

echo -----------------config main exe--------------------
"C:\Program Files\CMake\bin\cmake.exe" ^
-S%my_pwd% ^
--preset Ninja_release



echo -----------------config maya --------------------
"C:\Program Files\CMake\bin\cmake.exe" ^
-S%my_pwd% ^
--preset Ninja_release_maya_2018

"C:\Program Files\CMake\bin\cmake.exe" ^
-S%my_pwd% ^
--preset Ninja_release_maya_2019

"C:\Program Files\CMake\bin\cmake.exe" ^
-S%my_pwd% ^
--preset Ninja_release_maya_2020

echo -----------------build maya --------------------
"C:\Program Files\CMake\bin\cmake.exe" ^
--build ^
--preset release_maya_plug_2018
"C:\Program Files\CMake\bin\cmake.exe" ^
--build ^
--preset release_maya_plug_2019
"C:\Program Files\CMake\bin\cmake.exe" ^
--build ^
--preset release_maya_plug_2020



echo -----------------pack---------------------
"C:\Program Files\CMake\bin\cmake.exe" ^
--build ^
--preset gen_light_file

echo -----------------copy file--------------------
robocopy %my_pwd%build\Ninja_release\html %my_pwd%build\html /s /NFL /NDL
mkdir %my_pwd%build\html\file
robocopy %my_pwd%build\install %my_pwd%build\html\file *.msi
robocopy %my_pwd%build\install %my_pwd%build\html\file *.7z
py %my_pwd%build\generate_directory_index_caddystyle.py %my_pwd%\build\html\file

remdir %my_pwd%build\install\bin
remdir %my_pwd%build\install\maya
remdir %my_pwd%build\install\ue425_Plug
remdir %my_pwd%build\install\ue426_Plug
remdir %my_pwd%build\install\ue427_Plug

Exit 0
