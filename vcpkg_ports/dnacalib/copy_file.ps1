$UE_Root = "D:\Program Files\Epic Games\UE_5.7"
$RigLogicLib_Root = "$UE_Root\Engine\Plugins\Animation\RigLogic\Source\RigLogicLib"
$DNACalibLib = "$UE_Root\Engine\Plugins\Animation\DNACalib\Source\DNACalibLib"
$Root = Convert-Path "$PSScriptRoot\..\.."
$External = "$Root\external"
$External_RigLogicLib = "$External\RigLogicLib"
$External_DNACalibLib = "$External\DNACalibLib"
if (!(Test-Path -Path $External_RigLogicLib)) {
  New-Item -ItemType Directory -Path $External_RigLogicLib | Out-Null
}
if (!(Test-Path -Path $External_DNACalibLib)) {
  New-Item -ItemType Directory -Path $External_DNACalibLib | Out-Null
}


Copy-Item -Path "$RigLogicLib_Root\Private" -Destination $External_RigLogicLib -Recurse -Force
Copy-Item -Path "$RigLogicLib_Root\Public" -Destination $External_RigLogicLib -Recurse -Force

Remove-Item -Path "$External_RigLogicLib\Private\RigLogicLib.cpp"  -Force
Remove-Item -Path "$External_RigLogicLib\Public\RigLogicLib.h"  -Force
Remove-Item -Path "$External_RigLogicLib\Public\WindowsPlatformUE.h"  -Force
