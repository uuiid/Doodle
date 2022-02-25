set my_pwd=%~dp0
echo %my_pwd%

echo -----------------set env--------------------
call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat"

cd %my_pwd%

echo -----------------config--------------------

"C:\Program Files\CMake\bin\cmake.exe" ^
-S%my_pwd% ^
--preset=Ninja_release

echo -----------------build--------------------
"C:\Program Files\CMake\bin\cmake.exe" ^
--build ^
--preset cpack_doodle ^
--clean-first

echo -----------------copy file--------------------
robocopy %my_pwd%build\Ninja_release\html %my_pwd%build\html /s /NFL /NDL
mkdir %my_pwd%build\html\file
robocopy %my_pwd%build\Ninja_release %my_pwd%build\html\file *.msi /NFL /NDL
robocopy %my_pwd%build\Ninja_release %my_pwd%build\html\file *.7z /NFL /NDL

