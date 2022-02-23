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
robocopy %my_pwd%build\Ninja_release\html %my_pwd%build\html /s /e /NFL /NDL
mkdir %my_pwd%build\html\file
copy %my_pwd%build\Ninja_release\*.msi %my_pwd%build\html\file /y
copy %my_pwd%build\Ninja_release\*.7z %my_pwd%build\html\file /y

