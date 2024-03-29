set DoodleName=%1
robocopy //192.168.20.59/Doodle2/build/7z D:/7z /MIR

set DoodleTmpDir=D:\doodle_exe_tmp
set PATH=%PATH%;D:/7z

mkdir %DoodleTmpDir%

7za.exe x //192.168.20.59/Doodle2/build/Ninja_release/%DoodleName%.7z -o%DoodleTmpDir% -y
@REM net stop doodle_http_client_service

reg import //192.168.20.59/Doodle2/script/sy.reg

@REM 复制文件
robocopy %DoodleTmpDir%/%DoodleName% D:/doodle_exe/ /MIR /xd cgru
rmdir /Q /S %DoodleTmpDir%

@REM net start doodle_http_client_service

