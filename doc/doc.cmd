set my_pwd=%~dp0
echo %my_pwd%

echo -----------------set env--------------------
call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat"

cd %my_pwd%/..

call ./.venv/Scripts/activate.bat

echo -----------------build doxygen--------------------
"C:\Program Files\CMake\bin\cmake.exe" ^
--build ^
--preset doxygen

echo -----------------copy file--------------------
robocopy build\Ninja_release\html build\html /s /NFL /NDL

py ./doc/generate_directory_index_caddystyle.py %my_pwd%\..\build\html\file
py ./doc/generate_updata_log.py %my_pwd%\..\build\html\update.html
Exit 0
