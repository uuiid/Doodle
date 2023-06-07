# -*- coding: utf-8 -*-
import shelfBase
import maya.cmds as cmds
from maya import mel
import scripts.Doodle_PolyRemesh as Doodle_PolyRemesh
import scripts.Doodle_clear as Doodle_clear
import scripts.dem_cloth_to_fbx as dem_cloth_to_fbx
# import scripts.Doodle_deleteSurplusWeight as deleteWeight
import scripts.Doodle_deleteAttr as deleteAttr
import scripts.Doodle_blend_keyframe
import scripts.create_hair_uv as hair_uv
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
                      command="")
        self.addButon("af", "icons/doodle_afterimage.png",
                      command=cmds.doodle_afterimage)
        self.addButon("ql_rest", "icons/ql_rest.png",
                      command=cmds.doodle_duplicate_poly)
        self.addButon("set cache", "icons/set_cache.png",
                      command=lambda: self.set_cache())

        self.addButon("ik to fk", "icons/mark_ik_to_fk.png",
                      command=lambda: scripts.doodle_ik_to_fk.doodle_ik_to_fk())

        self.addButon("delect Mixed deformation attr", icon="icons/doodle_delete_attr",
                      command=self.deleteAttr)
        self.addButon("mesh to hair uv", icon="icons/hair_to_uv.png",
                      command=lambda: hair_uv.main())
        self.addButon("blend", icon="icons/blend.png",
                      command=self.blendkeyframe)

    def polyremesh(self):
        self.re()
        Doodle_PolyRemesh.myRemesh()

    def deleteAttr(self):
        self.re()
        deleteAttr.deleteShape().show()

    def exportAbc(self):
        cmds.ref_file_export()

    def set_cache(self):
        cmds.set_cloth_cache_path()

    def blendkeyframe(self):
        Doodle_blend_keyframe.backeProcess()

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
