call %~dp0/../build/set_venv.cmd

echo -----------------build main exe--------------------
"C:\Program Files\CMake\bin\cmake.exe" ^
--workflow --preset build_release%Doodle_suffix%