set my_pwd=%~dp0
echo %my_pwd%

echo -----------------set env--------------------
call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat"

cd %my_pwd%

echo -----------------config--------------------

"C:\Program Files\CMake\bin\cmake.exe" ^
-DCMAKE_BUILD_TYPE=Debug ^
-S%my_pwd% ^
-B%my_pwd%build/pw_build ^
--preset=Ninja_release

echo -----------------build--------------------
"C:\Program Files\CMake\bin\cmake.exe" ^
--build ^
--preset cpack_Doodle

echo -----------------copy file--------------------
mkdir %my_pwd%build/html/file/
robocopy %my_pwd%build/pw_build/html %my_pwd%build/html /s /mir
copy %my_pwd%build/pw_build/*.msi %my_pwd%build/html/file/


echo -----------------clear file--------------------
rmdir %my_pwd%build/pw_build /s