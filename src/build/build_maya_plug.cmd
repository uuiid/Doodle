call %~dp0/set_venv.cmd

echo -----------------------------------------------------
echo -----------------config maya 2018--------------------
"C:\Program Files\CMake\bin\cmake.exe" ^
-S%my_pwd% ^
--preset  Ninja_release%Doodle_suffix% -DMaya_Version=2018

if %errorlevel% NEQ 0 exit 1
echo -----------------build maya 2018--------------------
"C:\Program Files\CMake\bin\cmake.exe" ^
--build ^
--preset release_exe%Doodle_suffix% --target doodle_maya

if %errorlevel% NEQ 0 exit 1
echo -----------------install maya 2018--------------------
"C:\Program Files\CMake\bin\cmake.exe" ^
--install %my_pwd%\build\Ninja_release%Doodle_suffix% ^
--component maya_plug_com

if %errorlevel% NEQ 0 exit 1
echo -----------------clear maya 2018--------------------
powershell -command "$value = Get-Content %cache_file% -Encoding UTF8 | Select-String -Pattern 'maya' -NotMatch; Set-Content -Path %cache_file% -Value $value" 

 

echo -----------------------------------------------------
echo -----------------config maya 2019--------------------
"C:\Program Files\CMake\bin\cmake.exe" ^
-S%my_pwd% ^
--preset  Ninja_release%Doodle_suffix% -DMaya_Version=2019

if %errorlevel% NEQ 0 exit 1
echo -----------------build maya 2019--------------------
"C:\Program Files\CMake\bin\cmake.exe" ^
--build ^
--preset release_exe%Doodle_suffix% --target doodle_maya

if %errorlevel% NEQ 0 exit 1
echo -----------------install maya 2019--------------------
"C:\Program Files\CMake\bin\cmake.exe" ^
--install %my_pwd%\build\Ninja_release%Doodle_suffix% ^
--component maya_plug_com

if %errorlevel% NEQ 0 exit 1
echo -----------------clear maya 2019--------------------
powershell -command "$value = Get-Content %cache_file% -Encoding UTF8 | Select-String -Pattern 'maya' -NotMatch; Set-Content -Path %cache_file% -Value $value" 

echo -----------------------------------------------------
echo -----------------config maya 2020--------------------
"C:\Program Files\CMake\bin\cmake.exe" ^
-S%my_pwd% ^
--preset  Ninja_release%Doodle_suffix% -DMaya_Version=2020

if %errorlevel% NEQ 0 exit 1
echo -----------------build maya 2020--------------------
"C:\Program Files\CMake\bin\cmake.exe" ^
--build ^
--preset release_exe%Doodle_suffix% --target doodle_maya

if %errorlevel% NEQ 0 exit 1
echo -----------------install maya 2020--------------------
"C:\Program Files\CMake\bin\cmake.exe" ^
--install %my_pwd%\build\Ninja_release%Doodle_suffix% ^
--component maya_plug_com

if %errorlevel% NEQ 0 exit 1
echo -----------------clear maya 2020--------------------
powershell -command "$value = Get-Content %cache_file% -Encoding UTF8 | Select-String -Pattern 'maya' -NotMatch; Set-Content -Path %cache_file% -Value $value" 
