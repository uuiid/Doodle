# -*- coding: utf-8 -*-

import maya.standalone

maya.standalone.initialize(name='python')

import argparse
import maya.cmds
import maya.mel
import pymel.core
import os
import re
import json


parser = argparse.ArgumentParser(description="exportMayaFile")
parser.add_argument("--name", "-n", help="name attr")
parser.add_argument("--path", "-p", help="path attr")
parser.add_argument("--exportpath", "-exp", help="export path attr")
parser.add_argument("--version", "-v", help="version", default=0)
parser.add_argument("--suffix", "-su", help="suffix", default=".ma")
args = parser.parse_args()



maya.cmds.file(new=True, force=True)
maya.cmds.file(os.path.join(args.path, args.name + args.suffix), o=True)


def camBakeAim():
    cam = pymel.core.ls(sl=True)[0]
    # print cam.nodeName()
    # print type(cam)
    if (cam):
        # focalLen = cam.getFocalLength()
        try:
            camloa = pymel.core.spaceLocator()
            # print camloa
            # print type(camloa)
            pymel.core.parent(camloa, cam)

            camloa.setTranslation([0, 0, 0])
            camloa.setRotation([0, 0, 0])

            newCam = pymel.core.createNode('camera').getParent()
            newCam.setDisplayResolution(True)
            newCam.setDisplayGateMask(True)
            maya.mel.eval('setAttr "{}.displayGateMaskOpacity" 1;'.format(cam.getShape().longName()))
            newCam.setOverscan(1)
            pymel.core.rename(newCam, cam.nodeName())
            # try:
            #     newCam.setOverscan(1)
            # except:
            #     pass
            # newCam.setFocalLength(focalLen)

            pointCon = pymel.core.pointConstraint(camloa, newCam)
            orientCon = pymel.core.orientConstraint(camloa, newCam)

            start = pymel.core.playbackOptions(query=True, min=True)
            end = pymel.core.playbackOptions(query=True, max=True)

            pymel.core.copyKey(cam, attribute='focalLength', option='curve')
            try:
                pymel.core.copyKey(cam, attribute='focalLength', option='curve')
            except:
                focalLen = cam.getFocalLength()
            else:
                try:
                    pymel.core.pasteKey(newCam, attribute='focalLength')
                except:
                    focalLen = cam.getFocalLength()
                    newCam.setFocalLength(focalLen)
            pymel.core.bakeResults(newCam, sm=True, t=(start, end))

            pymel.core.delete(pointCon, orientCon, camloa)
            pymel.core.select(newCam)
        except:
            print("shi_Bai")
        else:
            print("cheng_Gong")


class export(object):

    def addfile(self, objname, path, version):
        setattr(self, objname, [path, version])


start = maya.cmds.playbackOptions(query=True, min=True)
end = maya.cmds.playbackOptions(query=True, max=True)

myfile = os.path.join(args.exportpath, "doodle_Export.json")
exports = maya.cmds.ls("::*UE4")
print("export select ->> {}".format(exports))
print("args exportpath --> {}".format(args.exportpath))
print("args name --> {}".format(args.name))
print("args version --> {}".format(args.version))
print("args suffix --> {}".format(args.suffix))

log = export()

for index,export in enumerate(exports):
    maya.cmds.select(export)
    split___ = export.split(":")[0].split("_")[0] + index.__str__()
    mel_name = "{path}/{name}_{suh}.fbx".format(path=args.exportpath, name=args.name, suh=split___)
    print("fbx export path --> {}".format(mel_name))
    maya.cmds.bakeResults(simulation=True, t=(start, end), hierarchy="below", sampleBy=1, disableImplicitControl=True,
                          preserveOutsideKeys=False, sparseAnimCurveBake=False)
    maya.mel.eval("FBXExportBakeComplexStart -v {}".format(start))
    maya.mel.eval("FBXExportBakeComplexEnd -v {}".format(end))
    maya.mel.eval("FBXExportBakeComplexAnimation -v true")
    maya.mel.eval("FBXExportSmoothingGroups -v true")
    maya.mel.eval("FBXExportConstraints -v true")
    maya.mel.eval('FBXExport -f "{}" -s'.format(mel_name))

    log.addfile(split___, mel_name, args.version)

exclude = ".*(front|persp|side|top|camera).*"

cameras = maya.cmds.ls(type='camera', l=True) or []

for camer in cameras:
    ex = True
    for test in filter(None, camer.split("|")):
        if re.findall(exclude, test):
            ex = False
    if not re.findall("_",camer):
        ex = False

    if ex == True:
        exportCamera = maya.cmds.listRelatives(camer, parent=True, fullPath=True)[0]
        maya.cmds.select(exportCamera)
        maya.cmds.bakeResults(simulation=True, t=(start, end), hierarchy="below", sampleBy=1,
                              disableImplicitControl=True, preserveOutsideKeys=False, sparseAnimCurveBake=False)
        if len(exportCamera.split("|")) > 2:
            camBakeAim()
        mel_name = "{path}/{name}_camera_{start}-{end}.fbx".format(path=args.exportpath, name=args.name, start=int(start),
                                                                   end=int(end))
        maya.mel.eval("FBXExportBakeComplexStart -v {}".format(start))
        maya.mel.eval("FBXExportBakeComplexEnd -v {}".format(end))
        maya.mel.eval("FBXExportBakeComplexAnimation -v true")
        maya.mel.eval("FBXExportConstraints -v true")
        maya.mel.eval('FBXExport -f "{}" -s'.format(mel_name))
        print("camera erport ----> {}".format(mel_name))
        log.addfile("camera", mel_name, args.version)

with open(myfile, "w") as f:
    f.write(json.dumps(log.__dict__,ensure_ascii=False, indent=4, separators=(',', ':')))
