# -*- coding: UTF-8 -*-
import sys
import os
import json
import re

import maya.mel
import pickle

from maya import OpenMayaUI as omui
from PySide2 import QtCore
from PySide2 import QtGui
from PySide2 import QtWidgets

from shiboken2 import wrapInstance

mayaMainWindowPtr = omui.MQtUtil.mainWindow()
mayaMainWindow = wrapInstance(long(mayaMainWindowPtr), QtWidgets.QWidget)


# def select_max_weiget():
#   import pymel.core
#   limt = 3
#   select = pymel.core.ls(selection=True, flatten=True, type="float3")
#   if not len(select) :
#      pymel.core.warning("No selected point")
#      return
#   skCluster = None
#   for node in select[0]._node.listHistory():
#       if node.type() == "skinCluster":
#           skCluster = node
#           break
#   selectList = [v for v in select if len([w for w in  pymel.core.skinPercent(skCluster, v, query=True, value=True) if w >0]) >limt]
#   pymel.core.select(selectList)

class deleteSurplusWeight(QtWidgets.QMainWindow):

    def __init__(self):
        super(deleteSurplusWeight, self).__init__()
        # 设置主窗口
        self.setParent(mayaMainWindow)
        self.setWindowTitle(QtWidgets.QApplication.translate("deleteSurplusWeight", "匹配ue权重", None, -1))
        self.resize(300, 60)
        # 设置中央小部件
        self.centralwidget = QtWidgets.QWidget(self)
        # 添加布局
        grid_layout = QtWidgets.QVBoxLayout(self.centralwidget)
        # 添加横向滑条布局
        hbox_layout = QtWidgets.QHBoxLayout(self.centralwidget)
        self.valueLable = QtWidgets.QLabel(self.centralwidget)
        self.valueLable.setText("7")
        hbox_layout.addWidget(self.valueLable)
        # 添加权重限制滑条
        self.limitWeight = QtWidgets.QSlider(self.centralwidget)
        self.limitWeight.setRange(4, 12)
        self.limitWeight.setValue(7)
        self.limitWeight.setOrientation(QtCore.Qt.Horizontal)
        self.limitWeight.valueChanged.connect(self.setLableValue)
        hbox_layout.addWidget(self.limitWeight)

        grid_layout.addLayout(hbox_layout)
        # 添加按钮删除权重按钮
        self.deleteWeightButten = QtWidgets.QPushButton(self.centralwidget)
        self.deleteWeightButten.setText(QtWidgets.QApplication.translate("deleteSurplusWeight", "删除权重", None, -1))
        self.deleteWeightButten.clicked.connect(self.deleteWight)
        grid_layout.addWidget(self.deleteWeightButten)
        # 添加获得删除权重点按钮
        self.getVexPoint = QtWidgets.QPushButton(self.centralwidget)
        self.getVexPoint.setText(QtWidgets.QApplication.translate("deleteSurplusWeight", "获得删除权重点", None, -1))
        self.getVexPoint.clicked.connect(self.selectDeleteWeightPoits)
        grid_layout.addWidget(self.getVexPoint)

        # 设置窗口标准
        self.setWindowFlags(QtCore.Qt.Window)
        self.centralwidget.setLayout(grid_layout)
        # 设置中央小部件
        self.setCentralWidget(self.centralwidget)
        # QtCore.QMetaObject.connectSlotsByName(self)
        self.selectList = []

    def setLableValue(self, value):
        self.valueLable.setText(value.__str__())

    def deleteWight(self):
        limt = self.limitWeight.value()
        print("============start delete weight=========================")
        import pymel.core
        select = pymel.core.ls(selection=True, flatten=True, type="float3")
        skCluster = None
        for node in select[0]._node.listHistory():
            if node.type() == "skinCluster":
                skCluster = node
                break
        bone = pymel.core.skinCluster(select[0]._node, query=True, weightedInfluence=True)
        self.selectList = []
        for vex in select:
            weight = pymel.core.skinPercent(skCluster, vex, query=True, value=True)
            weight = {key: i for key, i in enumerate(weight) if i > 0}
            if (weight.__len__() - limt) > 0:
                print("delete {} weight".format(vex))
                self.selectList.append(vex)
                weight = {b: pymel.core.skinPercent(skCluster, vex, query=True, transform=b) for b in bone}
                weight = {key: i for key, i in weight.items() if i > 0}
                weight_ = sorted(weight.items(), key=lambda item: item[1])
                deleteWei = weight_[:(weight_.__len__() - limt)]
                pymel.core.select(vex)
                pymel.core.skinPercent(skCluster, transformValue=[(i[0], 0) for i in deleteWei], normalize=True)
        print("============end delete weight=========================")

    def selectDeleteWeightPoits(self):
        pymel.core.select(self.selectList)
