# -*- coding: utf-8 -*-

import maya.standalone

maya.standalone.initialize(name='python')

import maya.cmds
import maya.mel
import pymel.core

import argparse
import os
import re
import json


parser = argparse.ArgumentParser(description="exportMayaFile")
parser.add_argument("--path", "-p", help="path attr")
parser.add_argument("--exportpath", "-exp", help="export path attr")

args = parser.parse_args()


maya.cmds.file(new=True, force=True)
maya.cmds.file(os.path.abspath(args.path), o=True)

doodle_filename = os.path.splitext(maya.cmds.file(
    query=True, sceneName=True, shortName=True))[0]

start = maya.cmds.playbackOptions(query=True, min=True)
end = maya.cmds.playbackOptions(query=True, max=True)

myfile = os.path.join(args.exportpath, "doodle_Export.json")
exports = maya.cmds.ls("::*UE4")
print("export select ->> {}".format(exports))
print("args exportpath --> {}".format(args.exportpath))


class camera:
    exclude = ".*(front|persp|side|top|camera).*"

    def __init__(self, cam):
        self.maya_cam = cam
        self.loa = pymel.core.spaceLocator()

    def export(self):
        ex = True
        for test in filter(None, self.maya_cam.fullPath().split("|")):
            if re.findall(self.exclude, test):
                ex = False
        if not re.findall("_", self.maya_cam.name()):
            ex = False

        # 如果不符合就直接返回
        if not ex:
            return

        if len(self.maya_cam.fullPath().split("|")) > 2:
            try:
                print("back anm")
                self.bakeAnm()
            except:
                print("back camera fail")
        else:
            pymel.core.bakeResults(self.maya_cam, sm=True, t=(start, end))

        mel_name = "{path}/{name}_camera_{start}-{end}.fbx".format(path=args.exportpath,
                                                                   name=doodle_filename,
                                                                   start=int(
                                                                       start),
                                                                   end=int(end))
        pymel.core.select(self.maya_cam)
        maya.mel.eval("FBXExportBakeComplexStart -v {}".format(start))
        maya.mel.eval("FBXExportBakeComplexEnd -v {}".format(end))
        maya.mel.eval("FBXExportBakeComplexAnimation -v true")
        maya.mel.eval("FBXExportConstraints -v true")
        maya.mel.eval('FBXExport -f "{}" -s'.format(mel_name))
        print("camera erport ----> {}".format(mel_name))
        log.addfile("camera", mel_name, 0)

    def bakeAnm(self):
        pymel.core.parent(self.loa, self.maya_cam)
        # 重置本地变换本地
        self.loa.setTranslation([0, 0, 0])
        self.loa.setRotation([0, 0, 0])

        self.newCam = pymel.core.createNode('camera').getParent()
        self.newCam.setDisplayResolution(True)
        self.newCam.setDisplayGateMask(True)

        print(self.maya_cam.getShape().longName())
        try:
            maya.mel.eval('setAttr "{}.displayGateMaskOpacity" 1;'.format(
                self.maya_cam.getShape().longName()))
        except:
            print("""Cannot set door mask transparency""")
        self.newCam.setOverscan(1)

        # 这个是最后要命名的新相机的名称
        name = self.maya_cam.nodeName()

        pointCon = pymel.core.pointConstraint(self.loa, self.newCam)
        orientCon = pymel.core.orientConstraint(self.loa, self.newCam)
        start = pymel.core.playbackOptions(query=True, min=True)
        end = pymel.core.playbackOptions(query=True, max=True)

        # pymel.core.copyKey(
        #     self.maya_cam, attribute='focalLength', option='curve')
        try:
            pymel.core.copyKey(
                self.maya_cam, attribute='focalLength', option='curve')
        except:
            print("not key is cam FocalLength")
        else:
            try:
                pymel.core.pasteKey(self.newCam, attribute='focalLength')
            except:
                focalLen = self.maya_cam.getFocalLength()
                self.newCam.setFocalLength(focalLen)

        pymel.core.bakeResults(self.newCam, sm=True, t=(start, end))

        pymel.core.delete(pointCon, orientCon, self.maya_cam)
        self.newCam.rename(name)
        self.maya_cam = self.newCam

    def __call__(self):
        try:
            self.export()
        except:
            print("not export")


class export(object):

    def addfile(self, objname, path, version):
        setattr(self, objname, [path, version])


log = export()

for index, export in enumerate(exports):
    try:
        maya.cmds.select(export)
        split___ = export.split(":")[0].split("_")[0] + index.__str__()
        mel_name = "{path}/{name}_{suh}.fbx".format(
            path=args.exportpath, name=doodle_filename, suh=split___)
        print("fbx export path --> {}".format(mel_name))
        maya.cmds.bakeResults(simulation=True,
                              t=(start, end),
                              hierarchy="below",
                              sampleBy=1,
                              disableImplicitControl=True,
                              preserveOutsideKeys=False,
                              sparseAnimCurveBake=False)
        maya.mel.eval("FBXExportBakeComplexStart -v {}".format(start))
        maya.mel.eval("FBXExportBakeComplexEnd -v {}".format(end))
        maya.mel.eval("FBXExportBakeComplexAnimation -v true")
        maya.mel.eval("FBXExportSmoothingGroups -v true")
        maya.mel.eval("FBXExportConstraints -v true")
        maya.mel.eval('FBXExport -f "{}" -s'.format(mel_name))

        log.addfile(split___, mel_name, 0)
    except:
        print("shibai --> " + export)

cameras = pymel.core.ls(type='camera', l=True)

for camer in cameras:
    camera(camer.getTransform())()

with open(myfile, "w") as f:
    f.write(json.dumps(log.__dict__, ensure_ascii=False,
                       indent=4, separators=(',', ':')))
