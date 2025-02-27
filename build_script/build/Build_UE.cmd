@echo off

if not exist "%my_pwd%/Doodle.code-workspace" goto set_pwd

if not exist "%cd%/Doodle.code-workspace" goto set_pwd

goto end

:set_pwd
cd %~dp0/../..
set my_pwd=%cd%

:end
call %my_pwd%/script/uePlug/build_5.cmd