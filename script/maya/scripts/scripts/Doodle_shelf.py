# -*- coding: utf-8 -*-
import shelfBase
import maya.cmds as cmds
from maya import mel
import scripts.Doodle_PolyRemesh as Doodle_PolyRemesh
import scripts.Doodle_clear as Doodle_clear
import scripts.dem_cloth_to_fbx as dem_cloth_to_fbx
# import scripts.Doodle_deleteSurplusWeight as deleteWeight
import scripts.Doodle_deleteAttr as deleteAttr
from PySide2 import QtCore
from PySide2 import QtGui
from PySide2 import QtWidgets
import random

import scripts.doodle_ik_to_fk


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

        button_color = self.addButon("randomColor", icon="icons/randomColor.png",
                                     command=lambda: self.randomColor(),
                                     doubleCommand=lambda: self.randomColor(
                                         self.__gen_color__),
                                     noDefaultPopup_=True)
        color_popup_menu = cmds.popupMenu(p=button_color, b=3)
        cmds.menuItem(label="Select colot",
                      parent=color_popup_menu,
                      command=lambda xx: self.randomColor(
                          self.__gen_color__))

        clear_button = self.addButon("clear", icon="icons/clear.png",
                                     command=lambda: self.print_r(cmds.doodle_clear_scene(
                                         dn=True, mu=True, uv=True,
                                         e1=True, e2=True, e3=True, e4=True)),
                                     noDefaultPopup_=True)
        clear_popup_menu = cmds.popupMenu(p=clear_button, b=3)
        cmds.menuItem(p=clear_popup_menu,
                      l=u"解锁全部法线",
                      command=lambda x: self.print_r(cmds.doodle_clear_scene(nn=True)))
        cmds.menuItem(p=clear_popup_menu,
                      l=u"选择重名物体",
                      command=lambda x: self.print_r(cmds.doodle_clear_scene(dn=True, sl=True)))
        cmds.menuItem(p=clear_popup_menu,
                      l=u"选择多边面",
                      command=lambda x: self.print_r(cmds.doodle_clear_scene(mu=True, sl=True)))
        cmds.menuItem(p=clear_popup_menu,
                      l=u"选择多uv",
                      command=lambda x: self.print_r(cmds.doodle_clear_scene(uv=True, sl=True)))

        self.addButon("hud", "icons/create_hud.png",
                      command=cmds.create_hud_node_maya)
        self.addButon("af", "icons/doodle_afterimage.png",
                      command=cmds.doodle_afterimage)
        self.addButon("ql_rest", "icons/ql_rest.png",
                      command=cmds.doodle_duplicate_poly)
        self.addButon("set cache", "icons/set_cache.png",
                      command=lambda: self.set_cache())
        self.addButon("export abc2", "icons/OUTabc2.png",
                      command=lambda: self._export_abc_and_upload_())

        self.addButon("ik to fk", "icons/mark_ik_to_fk.png",
                      command=lambda: scripts.doodle_ik_to_fk.doodle_ik_to_fk())
        self.addButon("abc to bl", "icons/sequence_to_blend_shape.png",
                      command=lambda: DlsShelf._export_cloth_fbx_())
        self.addButon("delect Mixed deformation attr", icon="icons/doodle_delete_attr",
                      command=self.deleteAttr)

    def polyremesh(self):
        self.re()
        Doodle_PolyRemesh.myRemesh()

    def deleteAttr(self):
        self.re()
        deleteAttr.deleteShape().show()

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

    def set_cache(self):
        cmds.doodle_create_ref_file()
        cmds.doodle_set_cloth_cache_path()

    @staticmethod
    def print_r(str_list):
        if "1" == str_list[0]:
            cmds.confirmDialog(b="ok", m=str_list[1])

    @staticmethod
    def randomColor(color_random_fun=shelfBase._null):
        select_lists = cmds.ls(sl=True)
        l_c = color_random_fun()

        for select_obj in select_lists:
            cmds.select(select_obj)
            l_c1 = l_c if l_c else [random.random() * .5,
                                    random.random() * .5,
                                    random.random() * .5]
            cmds.polyColorPerVertex(
                colorDisplayOption=True,
                colorR=l_c1[0],
                colorG=l_c1[1],
                colorB=l_c1[2])
        cmds.select(select_lists)

    @staticmethod
    def __gen_color__():
        l_r = cmds.colorEditor()
        if '1' == l_r.split()[3]:
            l_rgb = cmds.colorEditor(query=True, rgb=True)
            return l_rgb
        else:
            return [0.5, 0.5, 0.5]

    def _export_abc_and_upload_(self):
        cmds.doodle_create_ref_file()
        cmds.doodle_ref_file_export(
            startTime=1000, exportType="abc", select=True, force=True)
        cmds.doodle_upload_files()

    @staticmethod
    def _abc_to_fbx_():
        b_time = cmds.currentTime(q=True)
        select_list = cmds.ls(sl=True)
        if not select_list:
            return
        j_list = cmds.doodle_comm_dem_bones(select_list[0],
                                            bf=b_time)
        cmds.currentTime(b_time)
        l_du = cmds.duplicate(select_list[0], rr=True)
        j_list.append(l_du[0])
        cmds.skinCluster(j_list)
        cmds.doodle_comm_dem_bones_weiget(l_du)

    @staticmethod
    def _export_cloth_fbx_():
        l_select = cmds.ls(sl=True)
        cmds.doodle_create_ref_file()
        # cmds.doodle_ref_file_load()
        cmds.doodle_sequence_to_blend_shape_ref(
            startFrame=1001)
        cmds.select(l_select)
        cmds.doodle_ref_file_export(
            startTime=1001, exportType="fbx", select=True)
        cmds.doodle_upload_files()

    def re(self):
        key = QtWidgets.QApplication.keyboardModifiers()
        if key == QtCore.Qt.ShiftModifier:
            reload(Doodle_PolyRemesh)
            reload(Doodle_clear)
            # reload(deleteWeight)
            reload(deleteAttr)


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
                except RuntimeError as err:
                    print(err)
            DoodleUIManage._instances.discard(inst)
        gShelfTopLevel = mel.eval("global string $gShelfTopLevel; $tmp = $gShelfTopLevel;")
        gShelfTopLevel += "|Doodle"
        if cmds.shelfLayout(gShelfTopLevel, ex=1):
            try:
                cmds.deleteUI(gShelfTopLevel)
            except RuntimeError as err:
                print(err)
