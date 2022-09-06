call ./set_venv.cmd

"C:\Program Files\CMake\bin\cmake.exe" ^
--install %my_pwd%\build\Ninja_release ^
--component ue4_plug


if %errorlevel% NEQ 0 exit 1
