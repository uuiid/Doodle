set DoodleName=%1
set DoodleSource=//192.168.40.53/Doodle2/build/Ninja_release/_CPack_Packages/win64/7Z/%DoodleName%
set UE_CMD_PATH=C:/Program Files/Epic Games/UE_5.2/Engine/Binaries/Win64/UnrealEditor-Cmd.exe
reg import //192.168.40.53/Doodle2/script/sy.reg

@REM 复制文件
robocopy %DoodleSource% D:/doodle_exe/ /MIR /xd cgru

IF EXIST %UE_CMD_PATH% (
    set USER_DOODLE_PATH=%USERPROFILE%\Documents\doodle
    IF NOT EXIST %USER_DOODLE_PATH% mkdir %USER_DOODLE_PATH%
    copy \\192.168.40.53\doodle2\build_script\deploy\detail\doodle_config_v2 %USERPROFILE%\Documents\doodle\doodle_config_v2
)

@REM rmdir /Q /S D:\doodle
@REM net start doodle_http_client_service

