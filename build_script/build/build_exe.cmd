call %~dp0/set_venv.cmd

echo "Current Location is %my_pwd%"

del %cache_file%

echo -----------------config main exe--------------------
"C:\Program Files\CMake\bin\cmake.exe" ^
-S%my_pwd% ^
--preset Ninja_release%Doodle_suffix%

if %errorlevel% NEQ 0 exit 1

echo -----------------build main exe--------------------
"C:\Program Files\CMake\bin\cmake.exe" ^
--build ^
--preset Release_Exe%Doodle_suffix%


if %errorlevel% NEQ 0 exit 1
echo -----------------install main exe--------------------
"C:\Program Files\CMake\bin\cmake.exe" ^
--install %my_pwd%\build\Ninja_release%Doodle_suffix% ^
--component exe_com
@REM powershell -command "$value = Get-Content %cache_file% -Encoding UTF8 | Select-String -Pattern 'maya' -NotMatch; Set-Content -Path %cache_file% -Value $value" 
call  %my_pwd%\src\build\doc.cmd > %tmp%/doodle_doc_info.txt
