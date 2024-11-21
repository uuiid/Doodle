set DoodleName=%1
set DoodleSource=//192.168.20.89/Doodle2/build/Ninja_release/_CPack_Packages/win64/ZIP/%DoodleName%/bin


@REM 停止服务
net stop doodle_kitsu_supplement
@REM 复制文件
robocopy %DoodleSource% D:/kitsu/bin /MIR /xd dist
@REM 启动服务
net start doodle_kitsu_supplement
@REM 复制前端界面
robocopy //192.168.20.89/kitsu/dist D:/kitsu/dist /MIR