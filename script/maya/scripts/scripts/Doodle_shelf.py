# -*- coding: utf-8 -*-
import shelfBase
import maya.cmds as cmds

import scripts.Doodle_PolyRemesh as Doodle_PolyRemesh
import scripts.Doodle_clear as Doodle_clear
# import scripts.Doodle_deleteSurplusWeight as deleteWeight
# import scripts.Doodle_deleteAttr as deleteAttr
from PySide2 import QtCore
from PySide2 import QtGui
from PySide2 import QtWidgets


import maya_fun_tool as Doodle_fun_tool


class DlsShelf(shelfBase._shelf):
    cloth_to_fbx = None

    # def __init__(self, name="customShelf", iconPath=""):
    #     super(DlsShelf,self).__init__(name, iconPath)
    #     self.cloth_to_fbx = None

    def build(self):
        self.addButon("export_abc", icon="icons/OUTabc.png",
                      command=self.exportAbc)
        self.addButon("cam", icon="icons/OUTcam.png",
                      command=self.export_cam)
        self.addButon("OUTfbx", icon="icons/OUTfbx.png",
                      command=self.exportFbx)

        self.addButon("remesh", icon="icons/remesh.png",
                      command=self.polyremesh)

        # self.addButon("delect Weight", icon="icons/ue_delete_weight.png",
        #               command=self.deleteWeightPoint)
        # self.addButon("delect Mixed deformation attr", icon="icons/doodle_delete_attr",
        #               command=self.deleteAttr)

        self.addButon("randomColor", icon="icons/randomColor.png",
                      command=self.randomColor)
        self.addButon("hud", "icons/create_hud.png",
                      command=cmds.create_hud_node_maya)
        self.addButon("af", "icons/doodle_afterimage.png",
                      command=cmds.doodle_afterimage)

    def polyremesh(self):
        self.re()
        Doodle_PolyRemesh.myRemesh()

    def export_cam(self):
        cmds.doodle_export_camera()

    def exportAbc(self):
        cmds.doodle_create_ref_file()
        cmds.doodle_ref_file_export(
            startTime=1000, exportType="abc", select=True)

    def exportFbx(self):
        cmds.doodle_create_ref_file()
        cmds.doodle_ref_file_export(
            startTime=1001, exportType="fbx", select=True)

    # def deleteWeightPoint(self):
    #     self.re()
    #     deleteWeight.deleteSurplusWeight().show()

    # def deleteAttr(self):
    #     self.re()
    #     deleteAttr.deleteShape().show()

    def randomColor(self):
        import random
        select_lists = cmds.ls(sl=True)
        for select_obj in select_lists:
            cmds.select(select_obj)
            cmds.polyColorPerVertex(
                colorDisplayOption=True,
                colorR=random.random() * .5,
                colorG=random.random() * .5,
                colorB=random.random() * .5)

    def re(self):
        key = QtWidgets.QApplication.keyboardModifiers()
        if key == QtCore.Qt.ShiftModifier:
            reload(Doodle_PolyRemesh)
            reload(Doodle_clear)
            # reload(deleteWeight)
            # reload(deleteAttr)
            reload(Doodle_fun_tool)


class DoodleUIManage(object):
    _instances = set()

    @staticmethod
    def creation():
        shelf = DlsShelf("Doodle")
        DoodleUIManage._instances.add(shelf)

    @staticmethod
    def deleteSelf():
        for inst in tuple(DoodleUIManage._instances):
            if cmds.shelfLayout(inst.name, ex=1):
                try:
                    cmds.deleteUI(inst.name)
                except RuntimeError:
                    pass
            DoodleUIManage._instances.discard(inst)
