set UE_VERSION=5.2
set UE_5="D:\Program Files\Epic Games\UE_%UE_VERSION%\Engine\Build\BatchFiles\RunUAT.bat"
set work_dir=%~dp0
call %UE_5% BuildPlugin -Plugin=%work_dir%Doodle\Doodle.uplugin -TargetPlatforms=Win64 -Package=%work_dir%%UE_VERSION%\Plugins\Doodle -Rocket
rmdir /Q /S %work_dir%%UE_VERSION%\Plugins\Doodle\Intermediate
rmdir /Q /S %work_dir%%UE_VERSION%\Plugins\Doodle\Source