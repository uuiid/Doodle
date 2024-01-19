call %~dp0/set_venv.cmd

echo ----------------- build --------------------
"C:\Program Files\CMake\bin\cmake.exe" ^
--workflow --preset build_release%Doodle_suffix%

echo -----------------copy file--------------------
if not exist "%my_pwd%\build\html\file" goto create_file_dir
goto copy_file

:create_file_dir
mkdir %my_pwd%\build\html\file

:copy_file

echo -----------------copy file--------------------

robocopy %my_pwd%\build\Ninja_release %my_pwd%\build\html\file *.exe /ndl /np /njh /njs /ns /nc /nfl /m
robocopy %my_pwd%\build\Ninja_release %my_pwd%\build\html\file *.7z /ndl /np /njh /njs /ns /nc /nfl /m
robocopy %my_pwd%\build\Ninja_release\src\html %my_pwd%\build\html /s /ndl /np /njh /njs /ns /nc /nfl
python %my_pwd%/docs/generate_directory_index_caddystyle.py %my_pwd%/build/html/file
python %my_pwd%/docs/generate_updata_log.py %my_pwd%/build/html/update.html


