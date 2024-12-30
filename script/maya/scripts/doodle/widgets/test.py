
import maya.mel as mel
import sys

from PySide2 import QtWidgets

sys.path.append(r"E:\d\Python-project\pyside2-project")
import maya.cmds as cmds
import maya.OpenMayaUI as omui
from shiboken2 import wrapInstance
from doodle import doodle_maya_main_widget


def get_maya_main_window():
    main_window_ptr = omui.MQtUtil.mainWindow()
    return wrapInstance(long(main_window_ptr), QtWidgets.QWidget)


def my_custom_function(*args):
    parent = get_maya_main_window()
    win = doodle_maya_main_widget.MainWidget(parent)
    win.setWindowTitle(U"自定义工具窗口")
    win.show()


if cmds.window('MayaWindow', exists=True):
    main_maya_window = mel.eval('$temp1=$gMainWindow')
    menu_name = "doodle"
    if not cmds.menu(menu_name, exists=True):
        cmds.menu(menu_name, label="doodle", parent=main_maya_window)
        cmds.menuItem(label="c_doodle_item", parent=menu_name, command=my_custom_function)

else:
    print "Maya main window does not exist."
