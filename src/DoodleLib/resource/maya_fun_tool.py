import maya.cmds
import maya.mel
import pymel.core
from pymel.core.modeling import bevel
import pymel.core.system
import argparse
import os
import re
import json
import pymel.util.path


def get_file_name():
    return os.path.splitext(maya.cmds.file(
        query=True, sceneName=True, shortName=True))[0]


def create_maya_workspace():

    pass


class maya_play_raneg():
    def __init__(self):
        self.start = int(pymel.core.playbackOptions(query=True, min=True))
        self.end = int(pymel.core.playbackOptions(query=True, max=True))


class maya_file():
    def __init__(self):
        self.file_path = pymel.core.system.sceneName()
        self.maya_path_class = pymel.util.path(self.file_path)
        self.file_name = self.maya_path_class.basename()
        self.name_not_ex = self.file_name.splitext()[0]
        self.abs_path = self.maya_path_class.dirname()

    def save_as_cloth(self):
        pass


class maya_workspace():
    def __init__(self):
        self.work = pymel.core.system.workspace
        self.work.update()
        self.maya_file = maya_file()
        self.raneg = maya_play_raneg()
        k_work = self.maya_file.abs_path.dirname() / "workspace.mel"
        k_work2 = self.maya_file.abs_path / "workspace.mel"
        if(k_work.exists()):
            self.work.open(k_work.dirname())
        elif(k_work2.exists()):
            self.work.open(k_work2.dirname())
        else:
            self.work.open(self.maya_file.abs_path)
            self.work.fileRules["cache"] = "cache"
            self.work.save()


class export_log(object):

    def addfile(self, objname, path, version):
        setattr(self, objname, [path, version])


doodle_work_space = maya_workspace()

log = export_log()


class camera:
    exclude = "(front|persp|side|top|camera)"

    def __init__(self):
        self.maya_cam = None
        self.filter_cam()
        self.loa = None

    def filter_cam(self):
        for cam in pymel.core.ls(type='camera', l=True):
            ex = True
            for test in filter(None, cam.fullPath().split("|")):
                if re.findall(self.exclude, test):
                    ex = False
            if not re.findall("_", cam.name()):
                ex = False
            print(cam.fullPath(), ex)
            if ex:
                self.maya_cam = cam.getTransform()
                print("select cam ", self.maya_cam)
                break
        print("not select cam ", self.maya_cam)

    def create_move(self):
        # 如果不符合就直接返回
        if not self.maya_cam:
            return
        
        

    def export(self, export_path):

        # 如果不符合就直接返回
        if not self.maya_cam:
            return

        if len(self.maya_cam.fullPath().split("|")) > 2:
            try:
                print("back anm")
                self.bakeAnm()
            except:
                print("back camera fail")
        else:
            pymel.core.bakeResults(self.maya_cam, sm=True,
                                   t=(doodle_work_space.raneg.start,
                                      doodle_work_space.raneg.end))

        mel_name = "{path}/{name}_camera_{start}-{end}.fbx".format(
            path=export_path,
            name=doodle_work_space.maya_file.name_not_ex,
            start=int(doodle_work_space.raneg.start),
            end=int(doodle_work_space.raneg.end))
        pymel.core.select(self.maya_cam)
        print("Prepare export path ", mel_name)
        maya.mel.eval(
            "FBXExportBakeComplexStart -v {}".format(doodle_work_space.raneg.start))
        maya.mel.eval(
            "FBXExportBakeComplexEnd -v {}".format(doodle_work_space.raneg.end))
        maya.mel.eval("FBXExportBakeComplexAnimation -v true")
        maya.mel.eval("FBXExportConstraints -v true")
        maya.mel.eval('FBXExport -f "{}" -s'.format(mel_name))
        print("camera erport ----> {}".format(mel_name))
        log.addfile("camera", mel_name, 0)

    def bakeAnm(self):
        self.loa = pymel.core.spaceLocator()

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

    def __call__(self, str=None):
        if not str:
            str = doodle_work_space.maya_file.abs_path
        self.export(str)
        try:
            pass
        except:
            print("not export")


class maya_obj():
    def __init__(self, str):
        self.maya_obj = pymel.core.ls(str)

    def export_cloth_abc(self):
        pass

    def export_cloth_fbx(self):
        pass
