import maya.cmds
import maya.mel
from PySide2 import QtCore
from PySide2 import QtGui
from PySide2 import QtWidgets

from maya import OpenMayaUI as omui
from shiboken2 import wrapInstance

mayaMainWindowPtr = omui.MQtUtil.mainWindow()
mayaMainWindow = wrapInstance(long(mayaMainWindowPtr), QtWidgets.QWidget)


class deleButten(QtWidgets.QPushButton):
    def __init__(self, parent):
        super(deleButten, self).__init__(parent)
        self.deleteAttr = None
        self.index = -1
        self.node = None

    def addDelete(self, index, attr, node):
        self.index = index
        self.deleteAttr = attr
        self.node = node

    def deleteAttrDef(self):
        # print(self.index)
        maya.mel.eval("blendShapeDeleteTargetGroup {} {}".format(
            self.node, self.index))
        print("delete : {}--> {}".format(self.node, self.deleteAttr))
        self.deleteLater()


class deleteShape(QtWidgets.QMainWindow):
    def __init__(self):
        super(deleteShape, self).__init__()
        self.setParent(mayaMainWindow)
        self.setWindowFlags(QtCore.Qt.Window)
        self.resize(400, 800)

        self.wimMain = QtWidgets.QWidget(mayaMainWindow)

        self.node = None
        self.deletebutten = {}

        self.scrollara = QtWidgets.QScrollArea(mayaMainWindow)
        self.scrollara.setWidgetResizable(True)

        self.centralWidget = QtWidgets.QWidget()
        self.Hbox = QtWidgets.QVBoxLayout(self.centralWidget)

        self.selectButten = QtWidgets.QPushButton(self.centralWidget)
        self.selectButten.setText("get select node")
        self.selectButten.clicked.connect(self.getSelectNode)
        self.Hbox.addWidget(self.selectButten)
        self.maxNone = QtWidgets.QVBoxLayout(self.centralWidget)
        self.Hbox.addStretch()

        self.scrollara.setWidget(self.centralWidget)
        self.scrollara.setMinimumHeight(20)
        self.setCentralWidget(self.scrollara)

    def adddeleteButten(self):
        for k, b in self.deletebutten.items():
            b.deleteLater()

        self.deletebutten = {}
        try:
            for i, name in self.get_weight().items():
                self.deletebutten[i] = deleButten(self.scrollara)
                self.deletebutten[i].setObjectName("deletebutten{}".format(name))
                self.deletebutten[i].setText("delet:{}".format(name))
                self.deletebutten[i].setMinimumHeight(15)
                self.deletebutten[i].addDelete(i, i, self.node)
                self.deletebutten[i].clicked.connect(self.deletebutten[i].deleteAttrDef)
                self.Hbox.addWidget(self.deletebutten[i])
        except BaseException as err:
            maya.cmds.warning("err {}".format(err))
            maya.cmds.warning("Please select deformation node")

    def getSelectNode(self):
        l_select_ = maya.cmds.ls(sl=True)
        for i in l_select_:
            self.node = i
            self.adddeleteButten()
            break

    def get_weight(self):
        # type()->{int:string}
        l_weight_list = {}
        for i in range(0, len(maya.cmds.getAttr(self.node + ".weight"))):
            l_weight_list[i] = maya.cmds.attributeName("{}.weight[{}]".format(self.node, i), n=1)
        return l_weight_list
