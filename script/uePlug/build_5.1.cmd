
set UE_5_1="D:\Program Files\Epic Games\UE_5.1\Engine\Build\BatchFiles\RunUAT.bat"
set work_dir=%~dp0
%UE_5_1% BuildPlugin -Plugin=%work_dir%\Doodle\Doodle.uplugin -TargetPlatforms=Win64 -Package=%work_dir%\5.2\Plugins\Doodle
del /Q /S %work_dir%\5.2\Plugins\Doodle\Intermediate
del /Q /S %work_dir%\5.2\Plugins\Doodle\Source