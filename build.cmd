set my_pwd=%~dp0
echo %my_pwd%

echo -----------------set env--------------------
call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat"

cd %my_pwd%

echo -----------------config--------------------

"C:\Program Files\CMake\bin\cmake.exe" ^
-S%my_pwd% ^
--preset Ninja_release

echo -----------------build--------------------
"C:\Program Files\CMake\bin\cmake.exe" ^
--build ^
--preset release_exe
@REM --clean-first

echo -----------------pack---------------------
cd %my_pwd%/build
"C:\Program Files\CMake\bin\cpack.exe" ^
--config ./MultiCPackConfig.cmake

echo -----------------copy file--------------------
@REM robocopy %my_pwd%build\Ninja_release\html %my_pwd%build\html /s /NFL /NDL
@REM mkdir %my_pwd%build\html\file
@REM robocopy %my_pwd%build\Ninja_release %my_pwd%build\html\file *.msi /NFL /NDL
@REM robocopy %my_pwd%build\Ninja_release %my_pwd%build\html\file *.7z /NFL /NDL

