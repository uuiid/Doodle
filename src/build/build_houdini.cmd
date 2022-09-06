call ./set_venv.cmd

"C:\Program Files\CMake\bin\cmake.exe" ^
--install %my_pwd%\build\Ninja_release ^
--component doodle_houdini_com


if %errorlevel% NEQ 0 exit 1
