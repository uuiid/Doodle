call %~dp0/../build/set_venv.cmd


@REM echo -----------------config main exe--------------------
@REM "C:\Program Files\CMake\bin\cmake.exe" ^
@REM -S%my_pwd% ^
@REM --preset Ninja_build_DD

@REM if %errorlevel% NEQ 0 exit 1

echo -----------------build main exe--------------------
"C:\Program Files\CMake\bin\cmake.exe" ^
--build ^
--preset debug_doodle_dingding ^
--target %*