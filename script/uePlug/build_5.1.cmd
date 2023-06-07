
set UE_5_1="D:\Program Files\Epic Games\UE_5.2\Engine\Build\BatchFiles\RunUAT.bat"
set work_dir=%~dp0
call %UE_5_1% BuildPlugin -Plugin=%work_dir%Doodle\Doodle.uplugin -TargetPlatforms=Win64 -Package=%work_dir%5.2\Plugins\Doodle -Rocket
rmdir /Q /S %work_dir%5.2\Plugins\Doodle\Intermediate
rmdir /Q /S %work_dir%5.2\Plugins\Doodle\Source