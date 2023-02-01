@echo off

if not exist "%my_pwd%/src/doodle_core" goto set_pwd

if not exist "%cd%/src/doodle_core" goto set_pwd

goto end

:set_pwd
echo -----------------set env--------------------
call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat"

cd %~dp0/../..

set my_pwd=%cd%
if [%1] == [] (set doodle_install_prefix=all) else (set doodle_install_prefix=%1)
set cache_file=%my_pwd%\build\Ninja_release%Doodle_suffix%\CMakeCache.txt
call %my_pwd%/.venv/Scripts/activate.bat

if exist "%my_pwd%/CMakeUserPresets.json" (set Doodle_suffix=_DD)

:end

echo "Current Location is %my_pwd%"
echo "doodle_install_prefix is %doodle_install_prefix%"
echo "build config Ninja_release%Doodle_suffix%"
echo "build target release_exe%Doodle_suffix%"
