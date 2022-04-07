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
@REM --clean-first


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

echo -----------------config ue4 --------------------
"C:\Program Files\CMake\bin\cmake.exe" ^
-S%my_pwd% ^
--preset ue4_release_27
echo -----------------build ue4 --------------------
"C:\Program Files\CMake\bin\cmake.exe" ^
--build ^
--preset release_ue4_27


echo -----------------pack---------------------
cd %my_pwd%/build
"C:\Program Files\CMake\bin\cpack.exe" ^
--config ./MultiCPackConfig.cmake

echo -----------------copy file--------------------
robocopy %my_pwd%build\Ninja_release\html %my_pwd%build\html /s /NFL /NDL
mkdir %my_pwd%build\html\file
robocopy %my_pwd%build %my_pwd%build\html\file *.msi /NFL /NDL
robocopy %my_pwd%build %my_pwd%build\html\file *.7z /NFL /NDL

Exit 0