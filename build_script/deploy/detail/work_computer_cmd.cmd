set DoodleName=%1
set DoodleTmpDir=D:\doodle_exe_tmp
set PATH=%PATH%;C:\Program Files\7-Zip\

7z.exe x //192.168.20.59/Doodle2/build/Ninja_release/%DoodleName%.7z -o%DoodleTmpDir% -y
@REM net stop doodle_http_client_service

reg import //192.168.20.59/Doodle2/script/sy.reg

@REM 复制文件
robocopy %DoodleTmpDir%/%DoodleName% D:/doodle_exe/ /MIR
rmdir /Q /S %DoodleTmpDir%

@REM net start doodle_http_client_service

