# coding=utf-8
# version = 1.21

import maya.cmds
import maya.mel
import pymel.core
import re
import os

from PySide2 import QtCore
from PySide2 import QtGui
from PySide2 import QtWidgets
import scripts.chick_maya_model

reload(scripts.chick_maya_model)


class exportUe:

    def __init__(self):
        # 加载需要的插件
        maya.cmds.loadPlugin('AbcExport.mll')
        maya.cmds.loadPlugin('AbcImport.mll')

        filename = maya.cmds.file(q=True, sn=True, shn=True)
        self.filename = filename.split('.')[0]
        self._eps = 0
        self._shot = 0
        self._shotab = ""
        self.start = int(maya.cmds.playbackOptions(query=True, min=True))
        self.end = int(maya.cmds.playbackOptions(query=True, max=True))

        self.path = "C:/"
        self.root = "W:/"
        self.name = "tmp.ma"
        self.selects = pymel.core.selected()

    def analyseFileName(self):
        name_parsing_ep = re.findall("ep\d+", self.filename)
        name_parsing_shot = re.findall("sc\d+[_BCD]", self.filename)
        if name_parsing_ep and name_parsing_shot:
            try:
                self._eps = int(name_parsing_ep[0][2:])
                self._shot = int(name_parsing_shot[0][2:-1])
                shotab = name_parsing_shot[0][-1:]
                if shotab != "_":
                    self._shotab = shotab
            except NameError:
                print("not get episodes and shots")

    def createPath(self, suffix, extype):
        for sel in self.selects:
            namesp = sel.namespace()
            if namesp:
                break
        try:
            namesp = namesp.split(":")[-2].replace("_", "-")
        except:
            namesp = ""

        if suffix not in ["abc", "fbx"]:
            return
        if self.selects[0] in pymel.core.ls(type="camera", l=True):
            namesp = ""
            extype = "cam"

        self.path = "{root_}/03_Workflow/shots/ep{eps:0>3d}/" \
                    "sc{shot:0>4d}{shotab}/Scenefiles/{dep}/{aim}" \
            .format(
                eps=self._eps,
                shot=self._shot,
                shotab=self._shotab,
                dep=self.filename.split("_")[3],
                aim=self.filename.split("_")[4],
                root_=self.root
            )
        self.makePath()
        filecom = self.filename.split("_")
        self.name = "{f1}_{f2}_{f3}_{f4}_{f5}_export-{oa}_{ns}_.{st}-{end}.{su}"\
            .format(
                f1=filecom[0],
                f2=filecom[1],
                f3=filecom[2],
                f4=filecom[3],
                f5=filecom[4],
                oa=extype,
                ns=namesp,
                st=self.start,
                end=self.end,
                su=suffix
            )
        print("path --> {}".format(self.path))
        print("name --> {}".format(self.name))

    def makePath(self):
        myisExis = os.path.exists(self.path)
        if not myisExis:
            os.makedirs(self.path)
            print(self.path + " ok")
        else:
            print(self.path + " yi Zai")

    def export(self, nump):
        self.selects = pymel.core.selected()
        if not self.selects:
            return
        if not self.getRoot():
            return
        self.analyseFileName()

        if nump == "two":
            self.createPath("fbx", "repair")
            # exMesh = pymel.core.duplicate(self.selects)
            # exMesh = pymel.core.polyUnite(exMesh)
            # pymel.core.currentTime(self.end, update=True, edit=True)
        else:
            self.createPath("fbx", "cam")
            pymel.core.mel.eval("FBXExportBakeComplexAnimation -v true")
            pymel.core.mel.eval("FBXExportSmoothingGroups -v true")
            pymel.core.mel.eval("FBXExportConstraints -v true")
            pymel.core.mel.FBXExport(
                f="{}/{}".format(self.path, self.name), s=True)

        if nump == "two":
            scripts.chick_maya_model.run()()
            # self.margeMatMesh()
            self.createPath("abc", "repair")

            exAbc = pymel.core.polyUnite(self.selects)[0]
            abcexmashs = "-root {}".format(exAbc.fullPathName())
            # abcexmashs = ""
            # for exmash in self.exAbc:
            #     abcexmashs = "{} -root {}".format(abcexmashs,
            #                                       exmash.fullPathName())
            # -stripNamespaces
            abcExportCom = """AbcExport -j "-frameRange {f1} {f2} -stripNamespaces -uvWrite -writeFaceSets -worldSpace -dataFormat ogawa {mash} -file {f0}" """ \
                .format(f0="{}/{}".format(self.path, self.name).replace("\\", "/"), f1=self.start, f2=self.end, mash=abcexmashs)
            print(abcExportCom)
            pymel.core.mel.eval(abcExportCom)
        print(self.path)

    def getRoot(self):
        box = QtWidgets.QMessageBox()
        dubu_Butten = QtWidgets.QPushButton()
        dubu_Butten.setText("duBuXiaoYao")
        box.addButton(dubu_Butten, QtWidgets.QMessageBox.AcceptRole)

        chan_butten = QtWidgets.QPushButton()
        chan_butten.setText("changAnHuanJie")
        box.addButton(chan_butten, QtWidgets.QMessageBox.AcceptRole)

        user = QtWidgets.QPushButton()
        user.setText("custom")
        box.addButton(user, QtWidgets.QMessageBox.AcceptRole)

        my_quit_ = QtWidgets.QPushButton()
        my_quit_.setText("quit")
        box.addButton(my_quit_, QtWidgets.QMessageBox.AcceptRole)

        box.exec_()
        while 1:
            if box.clickedButton() == dubu_Butten:
                self.root = "V:"
                break
            elif box.clickedButton() == chan_butten:
                self.root = "X:"
                break
            elif box.clickedButton() == user:
                self.root = QtWidgets.QFileDialog().getExistingDirectory()
                break
            elif box.clickedButton() == my_quit_:
                return False
        return True
