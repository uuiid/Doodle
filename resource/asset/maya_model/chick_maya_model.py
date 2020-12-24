# -*- coding: utf-8 -*-
import json
import re
import os
import pymel.core
import pymel.all
import sys
# # import inspect
# # __Doodle__ = os.path.dirname(inspect.getsourcefile(lambda: 0))
# # __Doodle__ = os.path.dirname(os.path.dirname(__Doodle__))
# # print("add path {}".format(__Doodle__))
# # sys.path.append(__Doodle__)
# import argsTools

# print("{} {}".format(argsTools.ARGS.path,
#                      argsTools.ARGS.exportpath))
# # maya.cmds.file(new=True, force=True)
# # maya.cmds.file(os.path.join(args.path), o=True)

# if os.path.exists(os.path.dirname(argsTools.ARGS.path) + "/workspace.mel"):
#     pymel.all.workspace.open(os.path.dirname(argsTools.ARGS.path))
#     pymel.all.workspace.save()

# pymel.core.newFile(force=True, type="mayaAscii")
# pymel.core.openFile(os.path.abspath(argsTools.ARGS.path), force=True)


class materal():
    name = "materal"
    shader = "shader"

    def __init__(self, maya_obj):
        # 这个obj是材质
        self.maya_obj = maya_obj

    def toJson(self):
        return dict(name=self.name, shader=self.shader)

    def _getShaderGroup_(self):
        self.shader_group = self.maya_obj.shadingGroups()

    def chick(self):
        print(self.maya_obj.name())
        print(self.shader_group)

    def isSurfaceMaterial(self):
        # 获得分类是表面材质
        if [c for c in self.maya_obj.classification() if "shader/surface" in c]:
            return True
        else:
            return False


class uvmap():
    MultipleUvMap = True
    name = []

    def __init__(self, maya_obj):
        self.maya_obj = maya_obj
        self._getMapName_()

    def _getMapName_(self):
        self.name = pymel.core.polyUVSet(self.maya_obj, q=True, allUVSets=True)
        if len(self.name) > 1:
            self.MultipleUvMap = False

    def toJson(self):
        return dict(name=self.name, MultipleUvMap=self.MultipleUvMap)


class geometryInfo:
    PentagonalSurface = True
    name = "geometry"
    map = []
    materals = []

    def __init__(self, maya_obj):
        self.maya_obj = maya_obj
        self._getMaterals_()
        self._getUvMap_()

    def _getMaterals_(self):
        pymel.core.select(self.maya_obj)
        pymel.core.hyperShade(shaderNetworksSelectMaterialNodes=True)
        shader = pymel.core.ls(sl=True, materials=True)
        self.materals = [materal(s) for s in shader]

    def _getUvMap_(self):
        self.map.append(uvmap(self.maya_obj))

    def toJson(self):
        return dict(
            PentagonalSurface=self.PentagonalSurface,
            name=self.name,
            map=self.map,
            materals=self.materals
        )


class JsonEncoder(json.JSONEncoder):
    def default(self, obj):
        if hasattr(obj, "toJson"):
            return obj.toJson()
        else:
            return json.JSONEncoder.default(self, obj)


class chickFile():
    def __init__(self):
        self.geometrys = []
        self.__chcik_geo__ = []
        pass

    def selectAllPolygons(self):
        self.geometrys = pymel.core.ls(geometry=True)
        self.__chcik_geo__ = [geometryInfo(g) for g in self.geometrys]

    def chickMateral(self):
        pass

    def chickUvMap(self):
        pass

    def __call__(self):
        self.selectAllPolygons()
        self.chickUvMap()
        self.chickMateral()
        print("mats ")
        print(self.materals)


chickFile()()
