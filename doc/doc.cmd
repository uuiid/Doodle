set my_pwd=%~dp0
echo %my_pwd%

echo -----------------set env--------------------
call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat"

cd %my_pwd%/..

echo -----------------config doxygen--------------------
"C:\Program Files\CMake\bin\cmake.exe" ^
-S. ^
--preset Ninja_release

echo -----------------build doxygen--------------------
"C:\Program Files\CMake\bin\cmake.exe" ^
--build ^
--preset doxygen

echo -----------------copy file--------------------
robocopy build\Ninja_release\html build\html /s /NFL /NDL

py .\build\generate_directory_index_caddystyle.py %my_pwd%\..\build\html\file
Exit 0
