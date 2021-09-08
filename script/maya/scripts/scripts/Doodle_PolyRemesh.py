# -*- coding: utf-8 -*-
import maya.cmds
import maya.mel


class myRemesh(object):
    meshs = []
    duplicate_mesh = []
    bond = []
    max_edge_length = 1.0

    def __init__(self):
        self.filepath = "D:\\tmp"
        self.max_edge_length = 1
        self.collapse_threshold = 50
        self.onf_file = True
        self.preserveHardEdges = False
        self.mywindow = maya.cmds.window(widthHeight=(200, 300), title="polyRemesh")
        maya.cmds.columnLayout(adjustableColumn=True)
        maya.cmds.button(label="Get selected objects", command=self._getSelectMesh)
        maya.cmds.floatField(annotation='Subdivision value', minValue=0.1, maxValue=10, step=0.05,
                             value=self.max_edge_length,
                             dragCommand=self._getMaxEdgeLength, changeCommand=self._getMaxEdgeLength)
        maya.cmds.floatField(annotation='Collapse Threshold', minValue=1, maxValue=80, step=1,
                             value=self.collapse_threshold,
                             dragCommand=self._getCollapseThreshold, changeCommand=self._getCollapseThreshold)
        maya.cmds.checkBox(label="preserveHardEdges", changeCommand=self._setpreserveHardEdges, value=self.preserveHardEdges)
        maya.cmds.button(label="Subdivision", command=self._MYremesh)
        maya.cmds.button(label="buid", command=self._bindSkin)
        maya.cmds.button(label="Copy skin", command=self._copyBindSkin)
        maya.cmds.button(label="export file path", command=self._filePath)
        maya.cmds.checkBox(label="one file", changeCommand=self._setOneFile, value=self.onf_file)
        self.file_path_lable = maya.cmds.text(label='D:\\tmp', align="left")
        self.file_path_lable_butten = maya.cmds.button(label="export file", command=self._exportObj)
        maya.cmds.setParent("..")
        maya.cmds.showWindow(self.mywindow)

    def _MYremesh(self, t):
        self.duplicate_mesh = []
        for mesh in self.meshs:
            try:
                mesh_dub = mesh.split(":")[-1]
            except IndexError:
                mesh_dub = mesh
            mesh_dub = maya.cmds.duplicate(mesh, name="{}_rems".format(mesh_dub))[0]
            self.duplicate_mesh.append(mesh_dub)
            # print(mesh_dub)
            maya.cmds.select(mesh_dub)
            if(self.preserveHardEdges):
                maya.cmds.polyRetopo(preserveHardEdges=self.preserveHardEdges)
            maya.cmds.polyRemesh(maxEdgeLength=self.max_edge_length, collapseThreshold=self.collapse_threshold)

    def _getSelectMesh(self, t):
        self.meshs = []
        self.meshs = maya.cmds.ls(sl=True)
        # print(self.meshs)
        self.bond = []
        for mesh in self.meshs:
            self.bond.append(maya.cmds.skinCluster(mesh, query=True, weightedInfluence=True))
        # print(self.bond)

    def _getMaxEdgeLength(self, f):
        self.max_edge_length = f
        # print(self.max_edge_length)

    def _setpreserveHardEdges(self,f):
        self.preserveHardEdges = f;

    def _getCollapseThreshold(self, f):
        self.collapse_threshold = f

    def _setOneFile(self, f):
        # print(f)
        self.onf_file = f

    def _bindSkin(self, f):
        for index, m in enumerate(self.duplicate_mesh):
            maya.cmds.skinCluster(self.bond[index] + [m], toSelectedBones=True)

    def _copyBindSkin(self, f):
        for index, b in enumerate(self.duplicate_mesh):
            soure_sk = maya.mel.eval('findRelatedSkinCluster("{}")'.format(self.meshs[index]))
            trange_sk = maya.mel.eval('findRelatedSkinCluster("{}")'.format(b))
            maya.cmds.copySkinWeights(sourceSkin=soure_sk, destinationSkin=trange_sk, noMirror=True,
                                      surfaceAssociation="closestPoint",
                                      influenceAssociation=["closestJoint", "closestBone"])

    def _exportObj(self, f):
        if self.onf_file:
            mel_name = "{}/{}_v{:0>3d}.fbx".format(self.filepath, self.meshs[0].split(":")[0], 0)
            maya.cmds.select(self.duplicate_mesh + sum(self.bond,[]), replace=True)
            maya.mel.eval("FBXExportBakeComplexAnimation -v true")
            maya.mel.eval('FBXExport -f "%s" -s' % mel_name)
        else:
            for index, dumesh in enumerate(self.duplicate_mesh):
                mel_name = "{}/{}_v{:0>3d}.fbx".format(self.filepath, dumesh.split(":")[-1], index)
                maya.cmds.select([dumesh] + self.bond[index], replace=True)
                maya.mel.eval("FBXExportBakeComplexAnimation -v true")
                maya.mel.eval('FBXExport -f "%s" -s' % mel_name)

    def _filePath(self, f):
        self.filepath = maya.cmds.fileDialog2(fileMode=3, dialogStyle=1, caption="Open", fileFilter="allFiles(*.*)")[0]
        maya.cmds.text(self.file_path_lable, edit=True, label=self.filepath)
        # maya.cmds.button(self.file_path_lable_butten, edit=True, label=self.filepath)
        # print(self.filepath)
