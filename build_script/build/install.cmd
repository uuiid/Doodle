call %~dp0/set_venv.cmd

echo ----------------- build --------------------
"C:\Program Files\CMake\bin\cmake.exe" ^
--workflow --preset build_release%Doodle_suffix%

echo -----------------copy file--------------------
robocopy %my_pwd%\build\Ninja_release \\192.168.0.67\soft\TD软件\doodle *.exe /np /nfl
robocopy %my_pwd%\build\Ninja_release \\192.168.0.67\soft\TD软件\doodle *.7z /np /nfl


