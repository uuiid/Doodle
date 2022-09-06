if exist "%cd%/.git/config" goto end
if not  exist "%cd%/.git/config" goto set_pwd
goto end


:set_pwd
set my_pwd=%~dp0
echo %my_pwd%

echo -----------------set env--------------------
call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat"

cd %my_pwd%/../..

set my_pwd=%cd%
call %my_pwd%/.venv/Scripts/activate.bat

goto end

:end

echo "Current Location is %my_pwd%"
