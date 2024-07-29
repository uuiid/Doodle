set DoodleName=%1
set DoodleSource=//192.168.20.89/Doodle2/build/Ninja_release/_CPack_Packages/win64/7Z/%DoodleName%


@REM 停止服务
net stop doodle_file_association_http
net stop doodle_file_exists
net stop doodle_kitsu_supplement

@REM 复制文件
robocopy %DoodleSource% D:/doodle_exe/ /MIR

@REM 启动服务
net start doodle_file_association_http
net start doodle_file_exists
net start doodle_kitsu_supplement

