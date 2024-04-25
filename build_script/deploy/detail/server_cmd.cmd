set DoodleName=%1
set DoodleSource=//192.168.20.59/Doodle2/build/Ninja_release/_CPack_Packages/win64/7Z/%DoodleName%


@REM 停止服务
net stop doodle_scan_win_service
@REM net stop doodle_http_service

@REM 复制文件
robocopy %DoodleSource% D:/doodle_exe/ /MIR

@REM 启动服务
net start doodle_scan_win_service
@REM net start doodle_http_service

