@echo off

if exist "%my_pwd%/src/doodle_core" goto end
if not exist "%my_pwd%/src/doodle_core" goto set_pwd

goto end

:set_pwd
echo -----------------set env--------------------
call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat"

cd %~dp0/../..

set my_pwd=%cd%
call %my_pwd%/.venv/Scripts/activate.bat

goto end

:end

echo "Current Location is %my_pwd%"
