call ./set_venv.cmd

echo -----------------build doxygen--------------------
"C:\Program Files\CMake\bin\cmake.exe" ^
--build ^
--preset doxygen

echo -----------------copy file--------------------
robocopy build\Ninja_release\html build\html /s /NFL /NDL

py ./docs/generate_directory_index_caddystyle.py %my_pwd%\..\build\html\file
py ./docs/generate_updata_log.py %my_pwd%\..\build\html\update.html
Exit 0
