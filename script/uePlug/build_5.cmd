set UE_VERSION=5.5
set UE_5="D:\Program Files\Epic Games\UE_%UE_VERSION%\Engine\Build\BatchFiles\RunUAT.bat"
set work_dir=%~dp0
set l_time=%DATE:/=_%%TIME::=_%
set l_time=%l_time:.=_%
set l_time=%l_time: =%
call %UE_5% BuildPlugin -Plugin=%work_dir%Doodle\Doodle.uplugin -TargetPlatforms=Win64 -Package=%work_dir%%UE_VERSION%\Plugins\Doodle -Rocket
robocopy %work_dir%%UE_VERSION%\Plugins\Doodle %work_dir%%UE_VERSION%\Plugins\Doodle_%l_time% /mir /ndl /np /njh /njs /ns /nc /nfl
robocopy %work_dir%Doodle\ThirdParty %work_dir%%UE_VERSION%\Plugins\Doodle\ThirdParty /mir /ndl /np /njh /njs /ns /nc /nfl

rmdir /Q /S %work_dir%%UE_VERSION%\Plugins\Doodle\Intermediate
rmdir /Q /S %work_dir%%UE_VERSION%\Plugins\Doodle\Source
rmdir /Q /S %work_dir%..\..\build\pack\dist\Plugins

@REM call %UE_5% BuildPlugin -Plugin=%work_dir%UnrealEngine5VLC\VlcMedia.uplugin -TargetPlatforms=Win64 -Package=%work_dir%%UE_VERSION%\Plugins\UnrealEngine5VLC -Rocket
@REM robocopy %work_dir%\UnrealEngine5VLC\ThirdParty %work_dir%%UE_VERSION%\Plugins\UnrealEngine5VLC\ThirdParty /mir /ndl /np /njh /njs /ns /nc /nfl
@REM robocopy %work_dir%%UE_VERSION%\Plugins\UnrealEngine5VLC %work_dir%%UE_VERSION%\Plugins\UnrealEngine5VLC_%l_time% /mir /ndl /np /njh /njs /ns /nc /nfl

@REM rmdir /Q /S %work_dir%%UE_VERSION%\Plugins\UnrealEngine5VLC\Intermediate
@REM rmdir /Q /S %work_dir%%UE_VERSION%\Plugins\UnrealEngine5VLC\Source
