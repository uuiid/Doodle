call %~dp0/set_venv.cmd

echo ----------------- build --------------------
"C:\Program Files\CMake\bin\cmake.exe" ^
--workflow --preset build_debug


