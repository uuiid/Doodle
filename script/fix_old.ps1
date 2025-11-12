$Root = Convert-Path "$PSScriptRoot";
$MayaFile = Get-ChildItem -Path $Root -Filter "*.ma";
$MayaExe = Get-ItemPropertyValue -Path 'HKLM:SOFTWARE\Autodesk\Maya\2020\Setup\InstallPath' -Name MAYA_INSTALL_LOCATION
$MayaExe = Convert-Path "$MayaExe/bin/mayapy.exe"
$FixScript = "$env:TEMP/doodle_fix.py"
Write-Host "开始启动 $MayaExe 进行转换"

$MayaPyScript = @'
import pymel.core
import maya.cmds as cmds
import sys
print(sys.argv[1])
pymel.core.system.openFile(sys.argv[1], force=True)

l_ue_child = cmds.listRelatives("UE4", allDescendents=True, type="transform")
l_l = cmds.ls(type="qlClothShape")

l_export_set = set()


def list_p(in_list):
    l_list = [x for i in in_list for x in i]
    return list(set(l_list));

l_l =list_p(cmds.listHistory(i, future=True) for i in l_l)
l_l =list_p([cmds.ls(i, geometry=True)  for i in l_l])
l_l =list_p(cmds.listRelatives(i, parent=True) for i in l_l )
l_l =[i for i in l_l if i in l_ue_child]
print("set node ")
print(l_l)

for node in l_l:
    cmds.addAttr(node, longName="cloth", attributeType="bool", defaultValue=True, keyable=True)
cmds.file(save=True, type='mayaAscii')
cmds.quit()

quit()
'@

Set-Content $FixScript -Value $MayaPyScript -Encoding UTF8;

foreach ($item in $MayaFile) {
  $item = $item.FullName.Replace("\","/")
  Start-Process -FilePath $MayaExe -ArgumentList $FixScript, $item -WorkingDirectory $Root -NoNewWindow -Wait 
}