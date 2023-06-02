@echo off

if not exist "%my_pwd%/Doodle.code-workspace" goto set_pwd

if not exist "%cd%/Doodle.code-workspace" goto set_pwd

goto end

:set_pwd
echo -----------------set env--------------------
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"

cd %~dp0/../..

set my_pwd=%cd%
set cache_file=%my_pwd%\build\Ninja_release%Doodle_suffix%\CMakeCache.txt
call %my_pwd%/.venv/Scripts/activate.bat

@REM if exist "%my_pwd%/CMakeUserPresets.json" (set Doodle_suffix=_DD)

:end

echo "Current Location is %my_pwd%"
echo "build config Ninja_release%Doodle_suffix%"
echo "build target release_exe%Doodle_suffix%"
