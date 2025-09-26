import maya.mel as mel
from PySide2.QtWidgets import QWidget
import maya.cmds as cmds
import maya.OpenMayaUI as omui
from shiboken2 import wrapInstance
from doodle.doodle_maya_main_widget import MainWidget
from doodle.face_sculpting import FaceSculptingWidget

# from doodle.doodle_maya import State

win = None
face_sculpting_win = None


class main(object):

    @staticmethod
    def get_maya_main_window():
        main_window_ptr = omui.MQtUtil.mainWindow()
        return wrapInstance(long(main_window_ptr), QWidget)

    @staticmethod
    def add_doodle_tool_widget(*args):
        parent = main.get_maya_main_window()
        global win
        win = MainWidget(parent)
        win.setWindowTitle(U"DoodleTool")
        win.show()

    @staticmethod
    def add_face_sculpting_widget(*args):
        parent = main.get_maya_main_window()
        global face_sculpting_win
        face_sculpting_win = FaceSculptingWidget(parent)
        face_sculpting_win.setWindowTitle(U"FaceSculpting")
        face_sculpting_win.show()
        print('456')

    @staticmethod
    def add_menu():
        if cmds.window('MayaWindow', exists=True):
            main_maya_window = mel.eval('$temp1=$gMainWindow')
            menu_name = "Doodle"
            face_sculpting = "FaceSculpting"
            if not cmds.menu(menu_name, exists=True):
                cmds.menu(menu_name, label="Doodle", parent=main_maya_window)
                cmds.menuItem(label="Doodle Tool", parent=menu_name, command=main.add_doodle_tool_widget)
                cmds.menuItem(label=face_sculpting, parent=menu_name, command=main.add_face_sculpting_widget)
        else:
            print
            "Maya main window does not exist."

    @staticmethod
    def remove_menu():
        global win
        if win is not None:
            win.close()
            win.deleteLater()
        if face_sculpting_win is not None:
            face_sculpting_win.close()
            face_sculpting_win.deleteLater()

        cmds.deleteUI("Doodle", menu=True)
