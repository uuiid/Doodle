# -*- coding: utf-8 -*-
import json


import pymel.core
import pymel.all
import maya.mel

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
    isOk = False

    def __init__(self, maya_obj):
        # 这个obj是材质
        self.maya_obj = maya_obj
        self.name = maya_obj.name()
        self._getShaderGroup_()

    def toJson(self):
        return dict(name=self.name, shader=self.shader)

    def _getShaderGroup_(self):
        grp = self.maya_obj.shadingGroups()
        if grp:
            self.shader_group = self.maya_obj.shadingGroups()[0]
            self.shader = self.shader_group.name()

    def chick(self):
        print("_" * 20)
        print("materal name : {} \nshader group name: {} ".format(
            self.name,
            self.shader_group))
        # self.isOk = (self.shader_group == "{}SG".format(self.name))
        # print("is {} match".format(self.isOk))
        # if not self.isOk:
        # self.repairName()
        # self.repairMateralFacFe()
        self.reName()

    def repairName(self):
        try:
            self.shader_group.rename("{}SG".format(self.name))
            self.shader = self.shader_group.name()
        except:
            pass

    def reName(self):
        try:
            self.maya_obj.rename("{}Mat".format(self.name))
            self.shader_group.rename(self.name)
        except:
            print("-------->!!!!! shi_Bai")

    def repairMateralFace(self):
        polys = pymel.core.sets(self.shader_group, query=True)
        print("materal obj: ")
        for poly in polys:
            if poly.__class__.__name__ == "Mesh":
                pymel.core.sets(self.shader_group, remove=poly, edit=True)
                pymel.core.sets(self.shader_group, add=poly.faces, edit=True)

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
        print("_" * 20)
        print("get obj {}".format(self.maya_obj.name()))
        print("uv maps: ")
        print(self.name)

        if not self.name:
            return

        if len(self.name) > 1:
            self.MultipleUvMap = True
        else:
            self.MultipleUvMap = False
        print("set MultipleUvMap {}".format(self.MultipleUvMap))

    def toJson(self):
        return dict(name=self.name, MultipleUvMap=self.MultipleUvMap)


class geometryInfo:
    PentagonalSurface = True
    name = "geometry"
    map = None
    materals = []

    def __init__(self, maya_mesh_obj):
        self.maya_mesh_obj = maya_mesh_obj
        self.name = maya_mesh_obj.getTransform().name()
        self._getMaterals_()
        self._getUvMap_()

    def _getMaterals_(self):
        pymel.core.select(self.maya_mesh_obj, replace=True)
        # pymel.core.select(self.maya_mesh_obj.outputs())
        # pymel.core.select(clear=True)
        # for s in self.maya_mesh_obj.outputs():
        #     if(s.__class__ == pymel.all.ShadingEngine):
        #         pymel.core.select(s.surfaceShader.inputs(), add=True)
        # pymel.all.ShadingEngine
        # for s in self.maya_mesh_obj.outputs():
        #     pymel.core.select(s.surfaceShader.inputs(), add=True)
        pymel.core.hyperShade(shaderNetworksSelectMaterialNodes=True)
        for shd in pymel.core.selected(materials=True):
            if [c for c in shd.classification() if 'shader/surface' in c]:
                self.materals.append(materal(shd))

        if not self.materals:
            raise AssertionError()

    def _getUvMap_(self):
        self.map = uvmap(self.maya_mesh_obj)

    def chick(self):
        for materal in self.materals:
            try:
                materal.chick()
            except:
                pass

        pymel.core.select(self.maya_mesh_obj)
        # self.muFace = maya.mel.eval(
        #     "polyCleanup 1 1 1  0 1 0 0 0  0 0.1 0 0.1 0 0.1;")
        # if self.muFace:
        #     self.PentagonalSurface = False
        # else:
        #     self.PentagonalSurface = True

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
        self.geometrys = pymel.core.ls(sl=True)
        self.__chcik_geo__ = [geometryInfo(g) for g in self.geometrys]

    def chick(self):
        for poly in self.__chcik_geo__:
            poly.chick()

    def __call__(self, log_obj=None):
        print(self.geometrys)
        self.selectAllPolygons()
        self.chick()
        if log_obj:
            log_obj.log = json.dumps(self.__chcik_geo__, cls=JsonEncoder)
            log_obj.write()
        # pymel.core.saveFile()


run = chickFile
