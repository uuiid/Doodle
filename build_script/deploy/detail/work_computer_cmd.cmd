set DoodleName=%1
set DoodleSource=//192.168.20.59/Doodle2/build/Ninja_release/_CPack_Packages/win64/7Z/%DoodleName%

reg import //192.168.20.59/Doodle2/script/sy.reg

@REM 复制文件
robocopy %DoodleSource% D:/doodle_exe/ /MIR /xd cgru

@REM net start doodle_http_client_service

