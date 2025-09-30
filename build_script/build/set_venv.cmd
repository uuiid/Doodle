@echo off
chcp 65001
set VSLANG=1033
if not exist "%my_pwd%/Doodle.code-workspace" goto set_pwd

if not exist "%cd%/Doodle.code-workspace" goto set_pwd

goto end

:set_pwd
echo -----------------set env--------------------

cd %~dp0/../..
set my_pwd=%cd%

call %my_pwd%/.venv/Scripts/activate.bat
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"


:end


echo "Current Location is %my_pwd%"
echo "build config Ninja_release%Doodle_suffix%"
echo "build target release_exe%Doodle_suffix%"
echo "%*"

%*