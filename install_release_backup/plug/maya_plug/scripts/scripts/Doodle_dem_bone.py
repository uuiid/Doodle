# -*- coding: UTF-8 -*-
import sys
import os
import json
import re
import pymel.core
import maya.mel
import pickle
import UiFile.DleClothToFbx

from maya import OpenMayaUI as omui
from PySide2 import QtCore
from PySide2 import QtGui
from PySide2 import QtWidgets

from shiboken2 import wrapInstance
from multiprocessing import connection as Conn

reload(UiFile.DleClothToFbx)
mayaMainWindowPtr = omui.MQtUtil.mainWindow()
mayaMainWindow = wrapInstance(long(mayaMainWindowPtr), QtWidgets.QWidget)


class doodleSet(object):
    url = "getDoodleSet"


class convertSet(object):
    url = ""

    poly_meshname = ""
    convert_obj = None
    bones = 100
    Ofbx_filename = ""
    Ofbx_path = ""
    cluster_iter_num = 10
    global_iter_num = 30
    trans_iter_num = 5
    bind_update_num = 0
    trans_affine = 10
    trans_affine_norm = 4
    weights_iters = 3
    not_zero_bone_num = 8
    weights_smooth = 0.0001
    weights_smooth_step = 1

    def __init__(self):
        self.convert_node = None
        self.bind_obj = []
        self.convertJoint = []
        self.skinNode = None

    @property
    def Ofbx_filepath(self):
        path = os.path.abspath(os.path.join(self.Ofbx_path, self.Ofbx_filename)).replace("\\", "/")
        return path

    # def toCommand(self):
    #     list_com = ["DemBones.exe", "--abc=" + self.Iabc_filepath, "--init=" + self.Ifbx_filepath,
    #                 "--out=" + self.Ofbx_filepath.__str__(),
    #                 "--nBones=" + self.bones.__str__(), "--nInitIters=" + self.cluster_iter_num.__str__(),
    #                 "--nIters=" + self.global_iter_num.__str__(),
    #                 "--nTransIters=" + self.trans_iter_num.__str__(), "--bindUpdate=" + self.bind_update_num.__str__(),
    #                 "--transAffine=" + self.trans_affine.__str__(),
    #                 "--transAffineNorm=" + self.trans_affine_norm.__str__(),
    #                 "--nWeightsIters=" + self.weights_iters.__str__(), "--nnz=" + self.not_zero_bone_num.__str__(),
    #                 "--weightsSmooth=" + self.weights_smooth.__str__(),
    #                 "--weightsSmoothStep=" + self.weights_smooth_step.__str__()]
    #     return list_com

    # def to_dict(self):
    #     return {"Ifbx_filepath": self.Ifbx_filepath, "Iabc_filepath": self.Iabc_filepath,
    #             "Ofbx_filepath": self.Ofbx_filepath,
    #             "Command": self.toCommand()}

    def addSelectObj(self):
        pymel.core.select(self.bind_obj, add=True)

    def exportMesh(self, start, end):
        if not os.path.isdir(self.Ofbx_path):
            os.makedirs(self.Ofbx_path)
        pymel.core.select(self.convert_obj)
        maya.mel.eval("FBXExportBakeComplexStart -v {}".format(start))
        maya.mel.eval("FBXExportBakeComplexEnd -v {}".format(end))
        maya.mel.eval("FBXExportBakeComplexAnimation -v true")
        maya.mel.eval("FBXExportSmoothingGroups -v true")
        maya.mel.eval("FBXExportConstraints -v true")
        maya.mel.eval('FBXExport -f "{}" -s'.format(self.Ofbx_filepath))
        os.startfile(self.Ofbx_path)

    def _countBone_(self):
        num = pymel.core.polyEvaluate(self.convert_obj.name(), vertex=True)
        bone = int(num / 100)
        if bone < 35:
            bone = 35
        self.bones = bone
        return bone

    def createConvertNode(self):
        self.convert_node = pymel.core.createNode("doodleConvertBone")
        # .firstParent()
        self.convert_node.rename("{}_dolConvertBone".format(self.poly_meshname))
        self.convert_obj.getShape().outMesh >> self.convert_node.inputMesh
        self.convert_node.nBones.set(self._countBone_())
        self.convert_node.nIters.set(3)
        self.convert_node.startFrame.set(pymel.core.playbackOptions(query=True, min=True))
        self.convert_node.endFrame.set(pymel.core.playbackOptions(query=True, max=True) + 1)
        self.convert_node.bindFrame.set(pymel.core.playbackOptions(query=True, min=True) + 3)
        # self.convert_node.nBones.set(30)

    def playGetMesh(self):
        self.convert_node.getFrameData.get()

    def compute(self):
        if self.convert_node:
            self.convert_node.getOutPut.get()

    def createBind(self):
        if self.convert_node:
            pymel.core.currentTime(self.convert_node.bindFrame.get())
            self.bind_obj = self.convert_obj.duplicate()[0]
            pymel.core.parent(self.bind_obj, world=True)
            self.convertJoint = []
            bounding_box = self.bind_obj.boundingBox()
            size = pymel.core.datatypes.Vector(bounding_box[0] - bounding_box[1]).length()/self.convert_node.nBones.get()
            for i in range(self.convert_node.nBones.get()):
                jointnode = pymel.core.joint(None, name="{}_{:0>3d}".format(self.poly_meshname, i))
                jointnode.setTransformation(self.convert_node.localBindPoseList[i].localBindPose.get())
                jointnode.radius.set(size)
                self.convertJoint.append(jointnode)
            tmp = self.convertJoint[:]
            tmp.append(self.bind_obj)
            self.skinNode = pymel.core.skinCluster(tmp, toSelectedBones=True,polySmoothness=10.0)
            self.skinNode.normalizeWeights.set(0)
            # # 设置帧
            # for frame in range((self.convert_node.endFrame.get() - self.convert_node.startFrame.get())):
            #     pymel.core.currentTime(frame + self.convert_node.startFrame.get())
            #     for jone in range(self.convert_node.nBones.get()):
            #         value = self.convert_node.localAnimList[jone].localAnim[frame].get()
            #         self.convertJoint[jone].setTransformation(value)
            #         pymel.core.setKeyframe(self.convertJoint[jone])
            # 复制权重
            self._copyWeights_()

    def addParent(self, node):
        pymel.core.select(clear=True)
        group = pymel.core.group(self.convertJoint[:])
        group.rename("{}_grp".format(self.poly_meshname))
        pymel.core.parent([group, node], absolute=True)

    def _copyWeights_(self):
        pymel.core.doodleWeight(doodleConvertNode=self.convert_node,
                                doodleSkinCluster=self.skinNode)
        # evaluate = pymel.core.polyEvaluate(self.bind_obj, vertex=True)
        # for i in range(evaluate):
        #     pymel.core.select(self.bind_obj.vtx[i])
        #     value = []
        #     for jone in range(self.convert_node.nBones.get()):
        #         weight = self.convert_node.bindWeightsList[i].bindWeights[jone].get()
        #         value.append((self.convertJoint[jone], weight))
        #     # "{} :{}".format(prs, (i / evaluate) * 100)
        #     print("{po} {v:.3%} {po}".format(v=(float(i) / float(evaluate - 1)), po=("=" * 20)))
        #     pymel.core.skinPercent(self.skinNode, transformValue=value, normalize=False)


class DleClothToFbx(QtWidgets.QMainWindow, UiFile.DleClothToFbx.Ui_MainWindow):
    tran = []
    abc = []
    bem_bone = []
    _eps = 1
    _shot = 1
    _shotab = ""

    def __init__(self):
        super(DleClothToFbx, self).__init__()
        self.setParent(mayaMainWindow)
        self.setWindowFlags(QtCore.Qt.Window)

        self.setupUi(self)
        # 加载插件
        pymel.core.loadPlugin("fbxmaya")
        pymel.core.loadPlugin("AbcExport")
        pymel.core.loadPlugin("AbcImport")
        pymel.core.loadPlugin("AbcBullet")
        pymel.core.loadPlugin("DoodleConvertBone")

        # 设置启用
        self.createDoleConvert.setEnabled(False)
        self.getMeshGeo.setEnabled(False)
        self.computeButten.setEnabled(False)
        self.createBone.setEnabled(False)
        self.addParent.setEnabled(False)
        self.ExportClothAndFbx.setEnabled(False)

        # self.getSelectDynamicCloth.setEnabled(False)
        # self.selectdynamicClothList.setStyleSheet("background-color: rgb(60,60,60)")
        # 链接获取选择obj
        self.getSelectObj.clicked.connect(self._getSelectMesh)
        # 连接添加动态布料导出obj
        # self.getSelectDynamicCloth.clicked.connect(self._getSelectDynamMesh)

        # 添加创建函数
        self.createDoleConvert.clicked.connect(self.createDoleConvertNode)
        # 添加获得序列函数
        self.getMeshGeo.clicked.connect(self.getSqueueMeshData)
        # 添加计算触法函数
        self.computeButten.clicked.connect(self.compute)
        # 添加骨骼动画和绑定
        self.createBone.clicked.connect(self.createBindList)
        # 设置父物体
        self.addParent.clicked.connect(self.addParentList)
        # 解析文件名称
        self._getFileInfo()
        # 链接测试按钮
        self.testing.clicked.connect(self.clicledTesting)
        # 添加客户端
        self.client = Conn.Client(("127.0.0.1", 23369), authkey=b"doodle")

        # 添加导出连接
        self.ExportClothAndFbx.clicked.connect(self.exportButtenClicked)
        # 连接客户端获得必要信息
        self.client.send_bytes(pickle.dumps({"url": "getDoodleSet"}))
        doodleset = pickle.loads(self.client.recv_bytes())
        # 将信息写入实例
        self.cache_path = doodleset["cache_path"]
        self.user = doodleset["user"]
        self.user = doodleset["department"]
        self.projectname = doodleset["projectname"]

        # 添加动态布料启用
        # self.dynamicCloth.stateChanged.connect(self._setEnableDynamicCloth)

    def _getSelectMesh(self, a0):
        self.bem_bone = []
        self.tran = []
        self.tran = pymel.core.ls(sl=True)
        self.selectClothList.clear()
        self.selectClothList.addItems([t.name() for t in self.tran])
        self.ExportClothAndFbx.setEnabled(False)
        # 获得路径
        path = os.path.join(self.cache_path, self.getpath()[1:])
        self.outPath = path
        # 填写输出路径和名称
        print(path)
        for line, tran in enumerate(self.tran):
            bem_bone = convertSet()
            # 获得fbx网格名称和节点
            bem_bone.poly_meshname = tran.name().split(":")[-1]
            bem_bone.convert_obj = tran
            # 如果是动态特网格在一起就直接设置为一种
            bem_bone.abc_poly_meshname = tran.name()
            bem_bone.abc_obj = tran

            bem_bone.Ofbx_filename = tran.name() + u".fbx"

            bem_bone.Ofbx_path = path
            # bem_bone._countBone_(tran)
            self.bem_bone.append(bem_bone)

    def _getFileInfo(self):
        filename = pymel.core.system.sceneName()
        name_parsing_ep = re.findall("ep\d+", filename)
        name_parsing_shot = re.findall("sc\d+[_BCD]", filename)
        if name_parsing_ep and name_parsing_shot:
            try:
                self._eps = int(name_parsing_ep[0][2:])
            except NameError:
                self._eps = 1
            try:
                self._shot = int(name_parsing_shot[0][2:-1])
                shotab = name_parsing_shot[0][-1:]
                if shotab != "_":
                    self._shotab = shotab
                else:
                    self._shotab = ""
            except NameError:
                self._shot = 1
                self._shotab = ""
            self.episodes.setValue(self._eps)
            self.shot.setValue(self._shot)
            self.shotAb.setCurrentText(self._shotab)
        else:
            self.testing.setStyleSheet("background-color: darkred")
            # print(self.testing.text)
            self.testing.setText(u"检测(无法自动解析,请手动输入)")

    def getpath(self):
        my_dict = pickle.dumps(dict({"url": "getPath", "core": "shot"}, **self.pathInfo()))
        self.client.send_bytes(my_dict)
        return pickle.loads(self.client.recv_bytes())

    def pathInfo(self):
        return {"episodes": self._eps, "shot": self._shot, "shotab": self._shotab, "Type": "clothToFbx",
                "folder_type": "export_clothToFbx"}

    def computeConvertValue(self):
        pass

    def clicledTesting(self):
        self._getFileInfo()
        self.ScaneStartFrame = pymel.core.playbackOptions(query=True, min=True)
        self.ScaneEndFrame = pymel.core.playbackOptions(query=True, max=True) + 1

        self.ExportClothAndFbx.setEnabled(True)
        self.createDoleConvert.setEnabled(True)
        self.getMeshGeo.setEnabled(True)
        self.computeButten.setEnabled(True)
        self.createBone.setEnabled(True)
        self.addParent.setEnabled(True)

    def closeEvent(self, event):
        """
        关闭窗口时同时断开服务器连接
        :param event:
        :return:
        """
        self.client.send_bytes(b"close")
        self.client.close()

    def createDoleConvertNode(self):
        for conBone in self.bem_bone:
            conBone.createConvertNode()

    def getSqueueMeshData(self):
        for frame in range(int(self.ScaneEndFrame - self.ScaneStartFrame)):
            pymel.core.currentTime(int(self.ScaneStartFrame) + frame)
            for conBone in self.bem_bone:
                conBone.playGetMesh()

    def compute(self):
        for conBone in self.bem_bone:
            conBone.compute()

    def createBindList(self):
        for conBone in self.bem_bone:
            conBone.createBind()

    def addParentList(self):
        select_bone = pymel.core.ls(sl=True)[0]
        for conBone in self.bem_bone:
            conBone.addParent(select_bone)

    def exportButtenClicked(self):
        name = pymel.core.ls(sl=True)[0].name().split(":")[0].split("_")[0]
        for bem in self.bem_bone:
            bem.addSelectObj()
        maya.mel.eval("FBXExportBakeComplexStart -v {}".format(self.ScaneStartFrame))
        maya.mel.eval("FBXExportBakeComplexEnd -v {}".format(self.ScaneEndFrame))
        maya.mel.eval("FBXExportBakeComplexAnimation -v true")
        maya.mel.eval("FBXExportSmoothingGroups -v true")
        maya.mel.eval("FBXExportConstraints -v true")
        path = os.path.abspath(os.path.join(self.outPath, name)).replace("\\", "/")
        print(path)
        maya.mel.eval('FBXExport -f "{}" -s'.format(path))
        os.startfile(self.outPath)

        # if not (os.path.isfile(bem.Ifbx_filepath) and os.path.isfile(bem.Iabc_filepath)):
        #     is_export_ok = False

        # if is_export_ok:
        #     for ben in self.bem_bone:
        #         os.system("C:\\PROGRA~1\\doodle\\tools\\dem_bones\\" + " ".join(ben.toCommand()))
        #         pymel.core.FBXImportSkins("-v", True)
        #         pymel.core.FBXImportMode("-v", "add")
        #         pymel.core.FBXImport("-file", ben.Ofbx_filepath)
        #     self.client.send_bytes(pickle.dumps(
        #         {"url": "subInfo", "core": "shot", "specific": "subClothExport",
        #          "info": dict({"filepath": [bem.Ifbx_filepath for bem in self.bem_bone] +
        #                                    [bem.Iabc_filepath for bem in self.bem_bone]}, **self.pathInfo()),
        #          "_data_": [bem.to_dict() for bem in self.bem_bone]}))
        # self.client.send_bytes(b"close")
        # self.client.close()
        # self.close()
