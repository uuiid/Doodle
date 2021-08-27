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
        # self.addButon("export_cam", icon="icons/OUTcam.png", command=self.exportCam)
        self.addButon("export_abc", icon="icons/OUTabc.png",
                      command=self.exportAbc)
        self.addButon("back_cam", icon="icons/back_cam.png",
                      command=self.BakeAimCam)

        self.addButon("remesh", icon="icons/remesh.png",
                      command=self.polyremesh)

        self.addButon("clear", icon="icons/clear.png", command=self.clearScane)

        self.addButon("delect Weight", icon="icons/ue_delete_weight.png",
                      command=self.deleteWeightPoint)
        self.addButon("delect Mixed deformation attr", icon="icons/doodle_delete_attr",
                      command=self.deleteAttr)

        self.addButon("randomColor", icon="icons/randomColor.png",
                      command=self.randomColor)
        self.addButon("mark_sim", icon="icons/mark_sim.png",
                      command=self.mark_sim)

    def polyremesh(self):
        self.re()
        Doodle_PolyRemesh.myRemesh()

    def exportCam(self):
        cam = self.get_tool_cam()
        if cam:
            cam()

    def exportAbc(self):
        self.re()
        ref_file = None  # type: Doodle_fun_tool.references_file
        k_map = {"Q:/": "人间最得意",
                 "R:/": "万古神话",
                 "S:/": "藏锋",
                 "T:/": "狂神魔尊",
                 "U:/": "万域封神",
                 "V:/": "独步逍遥v3",
                 "X:/": "长安幻街",
                 "y_abc": "本地",
                 "z_quit": "取消"}
        box = QtWidgets.QMessageBox()
        buttens = {}  # type: map[str,QtWidgets.QPushButton]
        for p, n in k_map.iteritems():
            butten = QtWidgets.QPushButton()
            butten.setText(n)
            buttens[butten] = p
            box.addButton(butten, QtWidgets.QMessageBox.AcceptRole)

        box.exec_()
        path = pymel.core.Path()
        for b, p in buttens.iteritems():
            if b == box.clickedButton():
                path = p
        if str(path) == "z_quir":
            return
        elif str(path) == "y_abc":
            path = pymel.core.Path()
        else:
            path = pymel.core.Path(path)
            path = path / Doodle_fun_tool.analyseFileName().path()
        k_select = pymel.core.selected()

        if not k_select:
            return

        for s in k_select:
            ref = self.get_select_refFile(s)
            if ref:
                ref_file = Doodle_fun_tool.references_file(ref)

        if not ref_file:
            ref_file = Doodle_fun_tool.references_file(None)
            try:
                ref_file.namespace = k_select[0].namespaceList()[0]
            except IndexError:
                pymel.core.warning("not find namespace")

        ex = Doodle_fun_tool.cloth_group_file(ref_file)
        ex.export_select_abc(
            export_path=path, select_obj=k_select)

    def BakeAimCam(self):
        cam = self.get_tool_cam()
        if cam:
            cam.bakeAnm()

    def clearScane(self):
        self.re()
        Doodle_clear.clearAndUpload().clearScane()

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

    def mark_sim(self):
        select = pymel.core.selected()
        fls = set()  # type: set[pymel.core.system.FileReference]
        for s in select:
            ref = self.get_select_refFile()
            if ref:
                fls.add(pymel.core.FileReference(ref))
        print(fls)
        pymel.core.system.fileInfo["doodle_sim"] = str(fls)

    def get_select_refFile(self, maya_obj):
        # type: (Any)->pymel.core.FileReference
        try:
            ref = pymel.core.referenceQuery(
                maya_obj, referenceNode=True, topReference=True)
            if ref:
                return pymel.core.FileReference(ref)
            else:
                return
        except RuntimeError:
            return

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

    def get_tool_cam(self):
        # type: ()->Doodle_fun_tool.camera
        Doodle_fun_tool.doodle_work_space = Doodle_fun_tool.maya_workspace()
        Doodle_fun_tool.log = Doodle_fun_tool.export_log()

        cam = Doodle_fun_tool.camera()
        try:
            selects = pymel.core.selected()
            if not isinstance(selects[0].getShapes()[0], pymel.core.nodetypes.Camera):
                return
            cam.maya_cam = pymel.core.selected()[0]
            return cam
        except IndexError:
            pymel.core.warning("未选中有效cam")
            return
        except AttributeError:
            pymel.core.warning("选择物体类型不是cam")
            return


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
