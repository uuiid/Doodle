set DoodleName=%1
set DoodleTmpDir=D:\doodle_exe_tmp
set PATH=%PATH%;C:\Program Files\7-Zip\

7z.exe x //192.168.20.59/Doodle2/build/Ninja_release/%DoodleName%.7z -o%DoodleTmpDir% -y
@REM 停止服务
net stop doodle_scan_win_service
net stop doodle_http_service

@REM 复制文件
robocopy %DoodleTmpDir%/%DoodleName% D:/doodle_exe/ /MIR
rmdir /Q /S %DoodleTmpDir%

@REM 启动服务
net start doodle_scan_win_service
net start doodle_http_service

