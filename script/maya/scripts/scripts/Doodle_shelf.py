# -*- coding: utf-8 -*-
import shelfBase
import maya.cmds as cmds

import scripts.Doodle_PolyRemesh as Doodle_PolyRemesh
import scripts.Doodle_exportUe as Doodle_exportUe
import scripts.Doodle_cam as Doodle_cam
import scripts.Doodle_clear as Doodle_clear
import scripts.Doodle_dem_bone as Doodle_dem_bone
import scripts.Doodle_deleteSurplusWeight as deleteWeight
import scripts.Doodle_deleteAttr as deleteAttr
import scripts.export_usd as export_usd
from PySide2 import QtCore
from PySide2 import QtGui
from PySide2 import QtWidgets

import pymel.core
import pymel.core.system
import pymel.core.nodetypes
import maya_fun_tool as Doodle_fun_tool


class DlsShelf(shelfBase._shelf):
    cloth_to_fbx = None

    # def __init__(self, name="customShelf", iconPath=""):
    #     super(DlsShelf,self).__init__(name, iconPath)
    #     self.cloth_to_fbx = None

    def build(self):
        self.addButon("cam", icon="icons/OUTcam.png",
                      command=self.export_cam)
        self.addButon("export_abc", icon="icons/OUTabc.png",
                      command=self.exportAbc)
        self.addButon("OUTfbx", icon="icons/OUTfbx.png",
                      command=self.exportFbx)

        self.addButon("remesh", icon="icons/remesh.png",
                      command=self.polyremesh)

        self.addButon("delect Weight", icon="icons/ue_delete_weight.png",
                      command=self.deleteWeightPoint)
        self.addButon("delect Mixed deformation attr", icon="icons/doodle_delete_attr",
                      command=self.deleteAttr)

        self.addButon("randomColor", icon="icons/randomColor.png",
                      command=self.randomColor)
        self.addButon("hud", "icons/create_hud.png",
                      command=cmds.create_hud_node_maya)

    def polyremesh(self):
        self.re()
        Doodle_PolyRemesh.myRemesh()

    def export_cam(self):
        cmds.doodle_export_camera()

    def exportAbc(self):
        cmds.doodle_create_ref_file()
        cmds.doodle_ref_file_export(exportType="abc", select=True)

    def exportFbx(self):
        cmds.doodle_create_ref_file()
        cmds.doodle_ref_file_export(exportType="fbx", select=True)

    def deleteWeightPoint(self):
        self.re()
        deleteWeight.deleteSurplusWeight().show()

    def deleteAttr(self):
        self.re()
        deleteAttr.deleteShape().show()

    def randomColor(self):
        import pymel.core
        import random
        select_lists = pymel.core.ls(sl=True)
        for select_obj in select_lists:
            pymel.core.select(select_obj)
            pymel.core.polyColorPerVertex(
                colorDisplayOption=True,
                rgb=(random.random() * .5, random.random() * .5, random.random() * .5))

    def re(self):
        key = QtWidgets.QApplication.keyboardModifiers()
        if key == QtCore.Qt.ShiftModifier:
            reload(Doodle_PolyRemesh)
            reload(Doodle_exportUe)
            reload(Doodle_cam)
            reload(Doodle_clear)
            reload(Doodle_dem_bone)
            reload(deleteWeight)
            reload(deleteAttr)
            reload(export_usd)
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
