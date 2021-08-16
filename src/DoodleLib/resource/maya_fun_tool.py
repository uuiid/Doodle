import maya.cmds
import maya.mel
import pymel.core
import pymel.core.system
import pymel.core.nodetypes
import argparse
import os
import re
import json
import pymel.util.path
import pymel.all


def get_file_name():
    return os.path.splitext(maya.cmds.file(
        query=True, sceneName=True, shortName=True))[0]


def create_maya_workspace():

    pass


class maya_play_raneg():
    def __init__(self):
        self.start = int(pymel.core.playbackOptions(query=True, min=True))
        self.start = 1000
        self.end = int(pymel.core.playbackOptions(query=True, max=True))


class maya_file():
    def __init__(self):
        self.file_path = pymel.core.system.sceneName()
        self.maya_path_class = pymel.core.Path(
            self.file_path)  # type:  pymel.core.Path
        self.file_name = self.maya_path_class.basename()
        self.name_not_ex = self.file_name.splitext()[0]
        self.abs_path = self.maya_path_class.dirname()

    def save_as_cloth(self):
        pass


class maya_workspace():
    def __init__(self):
        self.work = pymel.core.system.workspace
        self.work.update()
        self.maya_file = maya_file()
        self.raneg = maya_play_raneg()
        k_work = self.maya_file.abs_path.dirname() / "workspace.mel"
        k_work2 = self.maya_file.abs_path / "workspace.mel"
        if(k_work.exists()):
            self.work.open(k_work.dirname())
        elif(k_work2.exists()):
            self.work.open(k_work2.dirname())
        else:
            self.work.open(self.maya_file.abs_path)
            self.work.fileRules["cache"] = "cache"
            self.work.save()


class export_log(object):

    def addfile(self, objname, path, version):
        setattr(self, objname, [path, version])


doodle_work_space = maya_workspace()

log = export_log()


class camera:
    exclude = "(front|persp|side|top|camera)"

    def __init__(self):
        self.maya_cam = None
        self.filter_cam()
        self.loa = None

    def filter_cam(self):
        for cam in pymel.core.ls(type='camera', l=True):
            ex = True
            for test in filter(None, cam.fullPath().split("|")):
                if re.findall(self.exclude, test):
                    ex = False
            if not re.findall("_", cam.name()):
                ex = False
            print(cam.fullPath(), ex)
            if ex:
                self.maya_cam = cam.getTransform()
                print("select cam ", self.maya_cam)
                break
        print("not select cam ", self.maya_cam)

    def create_move(self, out_path=None):
        # 如果不符合就直接返回
        if not self.maya_cam:
            return
        if not out_path:
            out_path = doodle_work_space.maya_file.abs_path
        tmp_path = pymel.core.Path(out_path)
        if not tmp_path.exists():
            tmp_path.mkdir_p()

        maya.mel.eval(
            "lookThroughModelPanel {} modelPanel1;".format(self.maya_cam))
        print("maya mel eval lookThroughModelPanel {} modelPanel1;".format(self.maya_cam))
        try:
            pymel.core.playblast(viewer=False,
                                 startTime=doodle_work_space.raneg.start,
                                 endTime=doodle_work_space.raneg.end,
                                 filename="{path}/{base_name}_playblast_{start}-{end}"
                                 .format(
                                     path=out_path,
                                     base_name=doodle_work_space.maya_file.name_not_ex,
                                     start=doodle_work_space.raneg.start,
                                     end=doodle_work_space.raneg.end
                                 ),
                                 percent=100,
                                 quality=100,
                                 offScreen=True,
                                 editorPanelName="modelPanel1",
                                 format="qt")
            pymel.core.system.warning("QuickTime not found, use default value")
        except RuntimeError:
            pymel.core.playblast(viewer=False,
                                 startTime=doodle_work_space.raneg.start,
                                 endTime=doodle_work_space.raneg.end,
                                 filename="{path}/{base_name}_playblast_{start}-{end}"
                                 .format(
                                     path=out_path,
                                     base_name=doodle_work_space.maya_file.name_not_ex,
                                     start=doodle_work_space.raneg.start,
                                     end=doodle_work_space.raneg.end
                                 ),
                                 percent=100,
                                 quality=100,
                                 offScreen=True,
                                 editorPanelName="modelPanel1",
                                 format="movie")

    def export(self, export_path):

        # 如果不符合就直接返回
        if not self.maya_cam:
            return
        tmp_path = pymel.core.Path(export_path)
        if not tmp_path.exists():
            tmp_path.mkdir_p()

        if len(self.maya_cam.fullPath().split("|")) > 2:
            try:
                print("back anm")
                self.bakeAnm()
            except:
                print("back camera fail")
        else:
            pymel.core.bakeResults(self.maya_cam, sm=True,
                                   t=(doodle_work_space.raneg.start,
                                      doodle_work_space.raneg.end))

        mel_name = "{path}/{name}_camera_{start}-{end}.fbx".format(
            path=export_path,
            name=doodle_work_space.maya_file.name_not_ex,
            start=int(doodle_work_space.raneg.start),
            end=int(doodle_work_space.raneg.end))
        pymel.core.select(self.maya_cam)
        print("Prepare export path ", mel_name)
        maya.mel.eval(
            "FBXExportBakeComplexStart -v {}".format(doodle_work_space.raneg.start))
        maya.mel.eval(
            "FBXExportBakeComplexEnd -v {}".format(doodle_work_space.raneg.end))
        maya.mel.eval("FBXExportBakeComplexAnimation -v true")
        maya.mel.eval("FBXExportConstraints -v true")
        maya.mel.eval('FBXExport -f "{}" -s'.format(mel_name))
        print("camera erport ----> {}".format(mel_name))
        log.addfile("camera", mel_name, 0)

    def bakeAnm(self):
        self.loa = pymel.core.spaceLocator()

        pymel.core.parent(self.loa, self.maya_cam)
        # 重置本地变换本地
        self.loa.setTranslation([0, 0, 0])
        self.loa.setRotation([0, 0, 0])

        self.newCam = pymel.core.createNode('camera').getParent()
        self.newCam.setDisplayResolution(True)
        self.newCam.setDisplayGateMask(True)

        print(self.maya_cam.getShape().longName())
        try:
            maya.mel.eval('setAttr "{}.displayGateMaskOpacity" 1;'.format(
                self.maya_cam.getShape().longName()))
        except:
            print("""Cannot set door mask transparency""")
        self.newCam.setOverscan(1)

        # 这个是最后要命名的新相机的名称
        name = self.maya_cam.nodeName()

        pointCon = pymel.core.pointConstraint(self.loa, self.newCam)
        orientCon = pymel.core.orientConstraint(self.loa, self.newCam)
        start = pymel.core.playbackOptions(query=True, min=True)
        end = pymel.core.playbackOptions(query=True, max=True)

        # pymel.core.copyKey(
        #     self.maya_cam, attribute='focalLength', option='curve')
        try:
            pymel.core.copyKey(
                self.maya_cam, attribute='focalLength', option='curve')
        except:
            print("not key is cam FocalLength")
        else:
            try:
                pymel.core.pasteKey(self.newCam, attribute='focalLength')
            except:
                focalLen = self.maya_cam.getFocalLength()
                self.newCam.setFocalLength(focalLen)

        pymel.core.bakeResults(self.newCam, sm=True, t=(start, end))

        pymel.core.delete(pointCon, orientCon, self.maya_cam)
        self.newCam.rename(name)
        self.maya_cam = self.newCam

    def __call__(self, str=None):
        if not str:
            str = doodle_work_space.maya_file.abs_path
        self.export(str)
        try:
            pass
        except:
            print("not export")


class meateral():
    def __init__(self, meateral_obj):
        self.maya_obj = meateral_obj
        self.name = self.mat.name()
        self.shader_group = None
        self.shader = None
        grp = self.maya_obj.shadingGroups()
        if grp:
            self.shader_group = self.maya_obj.shadingGroups()[0]
            self.shader = self.shader_group.name()

    def chick(self):
        self.reName()

    def reName(self):
        if self.maya_obj and self.shader_group:
            self.maya_obj.rename("{}Mat".format(self.name))
            self.shader_group.rename(self.name)
            print("rename {} ".format(self.name))
        else:
            print("not rename {}".format(self.maya_obj))

    def isSurfaceMaterial(self):
        # 获得分类是表面材质
        if [c for c in self.maya_obj.classification() if "shader/surface" in c]:
            return True
        else:
            return False

    def repairMateralFace(self):
        polys = pymel.core.sets(self.shader_group, query=True)
        print("materal obj: ")
        for poly in polys:
            if poly.__class__.__name__ == "Mesh":
                pymel.core.sets(self.shader_group, remove=poly, edit=True)
                pymel.core.sets(self.shader_group, add=poly.faces, edit=True)


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


class geometryInfo():
    def __init__(self, maya_mesh_obj):
        # type: (pymel.core.nodetypes.Transform)->None
        self.maya_mesh_obj  # type: pymel.core.nodetypes.Shape
        try:
            self.maya_mesh_obj = maya_mesh_obj.getShapes()[0]
        except AttributeError:
            self.maya_mesh_obj = maya_mesh_obj

        self.name = self.maya_mesh_obj.getTransform().name()
        self.out_path = doodle_work_space.maya_file.abs_path
        self.materals = []  # type: list[meateral]

        self._getMaterals_()

    def repair(self):
        for mat in self.materals:
            mat.reName()

    def _getMaterals_(self):
        pymel.core.select(self.maya_mesh_obj, replace=True)
        pymel.core.select(clear=True)
        for s in self.maya_mesh_obj.outputs():
            if(s.__class__ == pymel.all.ShadingEngine):
                pymel.core.select(s.surfaceShader.inputs(), add=True)
        for shd in pymel.core.selected(materials=True):
            if [c for c in shd.classification() if 'shader/surface' in c]:
                self.materals.append(meateral(shd))


class references_file():
    def __init__(self, ref_obj):
        # type:(pymel.core.FileReference)->None

        self.maya_ref = ref_obj
        self.path = self.maya_ref.path  # type: pymel.core.Path
        self.namespace = self.maya_ref.fullNamespace
        if self.path.fnmatch("*[rig].ma"):
            self.cloth_path = pymel.core.Path(
                "V:/03_Workflow/Assets/CFX/cloth") / self.path.name.replace(
                "rig", "cloth")
        else:
            self.cloth_path = None

    def replace_file(self):
        if(self.cloth_path):
            self.maya_ref.replaceWith(self.cloth_path)


class cloth_group_file():
    def __init__(self, ref_obj):
        # type:(references_file)->None
        self.maya_ref = ref_obj
        self.maya_name_space = ref_obj.namespace
        pymel.core.select(clear=True)
        pymel.core.select(
            "{}:*cloth_proxy".format(self.maya_name_space), replace=True)
        self.cloth_group = pymel.core.selected()

        self.maya_abc_export  # type: list[pymel.core.nodetypes.Transform]

    def qcloth_update_pose(self):
        for obj in self.cloth_group:
            select_str = obj.name(stripNamespace=True).replace(
                "cloth_proxy", "skin_proxy")
            pymel.core.select(
                "{}:*{}".format(self.maya_name_space, select_str), replace=True)
            pymel.core.select(obj, add=True)
            maya.mel.eval("qlUpdateInitialPose;")

    def set_cache_folder(self):
        for obj in self.cloth_group:
            select_str = obj.name(stripNamespace=True).replace(
                "cloth_proxy", "clothShape")
            pymel.core.select(
                "{}:*{}".format(self.maya_name_space, select_str), replace=True)
            qcloth_obj = pymel.core.selected()[0]
            path = doodle_work_space.maya_file.name_not_ex / self.maya_name_space / select_str
            doodle_work_space.work.mkdir(doodle_work_space.work.path / path)
            print(path)
            print(qcloth_obj)
            qcloth_obj.setAttr("cacheFolder", str(path))
            qcloth_obj.setAttr("cacheName", select_str)

    def hide_other(self):
        pymel.core.select("{}:cfx_grp".format(self.maya_name_space))
        pymel.core.hide(pymel.core.selected())

    def show_other(self):
        pymel.core.select("{}:cfx_grp".format(self.maya_name_space))
        pymel.core.hide(pymel.core.selected())

    def export_fbx(self):
        pymel.core.select("{}:*UE4".format(self.maya_name_space))
        # 空置就返回
        if not pymel.core.selected():
            return

        path = doodle_work_space.work.getPath() / "fbx"  # type: pymel.core.Path

        name = "{}_{}_{}-{}.fbx".format(doodle_work_space.maya_file.name_not_ex,
                                        self.maya_name_space,
                                        doodle_work_space.raneg.start,
                                        doodle_work_space.raneg.end)
        path.mkdir_p()
        path = path / name
        print("export path : {}".format(path))
        pymel.core.bakeResults(simulation=True,
                               time=(doodle_work_space.raneg.start,
                                     doodle_work_space.raneg.end),
                               hierarchy="below",
                               sampleBy=1,
                               disableImplicitControl=True,
                               preserveOutsideKeys=False,
                               sparseAnimCurveBake=False)
        maya.mel.eval(
            "FBXExportBakeComplexStart -v {}".format(doodle_work_space.raneg.start))
        maya.mel.eval(
            "FBXExportBakeComplexEnd -v {}".format(doodle_work_space.raneg.end))
        maya.mel.eval("FBXExportBakeComplexAnimation -v true")
        maya.mel.eval("FBXExportSmoothingGroups -v true")
        maya.mel.eval("FBXExportConstraints -v true")
        maya.mel.eval('FBXExport -f "{}" -s'.format(path))

    def export_abc(self):
        # 选择物体
        self.maya_ref.maya_ref.importContents()
        pymel.core.select("{}:*_wrap")

        # 创建路径
        path = doodle_work_space.work.getPath() / "abc"  # type: pymel.core.Path
        name = "{}_{}_{}-{}.abc".format(doodle_work_space.maya_file.name_not_ex,
                                        self.maya_name_space,
                                        doodle_work_space.raneg.start,
                                        doodle_work_space.raneg.end)
        path.mkdir_p()
        path = path / name
        print("export path : {}".format(path))

        # 导出物体
        # list[pymel.core.nodetypes.Transform]
        self.maya_abc_export = pymel.core.selected()

        if not self.maya_abc_export:
            return

        for obj in self.maya_abc_export:
            k_geo = geometryInfo(obj)
            k_geo.repair()
        export_abc = None  # type: pymel.core.nodetypes.Transform
        if len(self.maya_abc_export) > 1:
            export_abc = pymel.core.polyUnite(self.maya_abc_export)[0]
        else:
            export_abc = self.maya_abc_export[0]

        abcexmashs = "-root {}".format(export_abc.fullPathName())
        # abcexmashs = ""
        # for exmash in self.export_abc:
        #     abcexmashs = "{} -root {}".format(abcexmashs,
        #                                       exmash.fullPathName())
        # -stripNamespaces
        abcExportCom = """AbcExport -j "-frameRange {f1} {f2} -stripNamespaces -uvWrite -writeFaceSets -worldSpace -dataFormat ogawa {mash} -file {f0}" """ \
            .format(f0=path,
                    f1=doodle_work_space.raneg.start, f2=doodle_work_space.raneg.end,
                    mash=abcexmashs)
        print(abcExportCom)
        pymel.core.mel.eval(abcExportCom)


class cloth_export():
    def __init__(self):

        self.colth = []  # type: list[references_file]
        self.qcolth_group = []  # type: list[cloth_group_file]
        for ref_obj in pymel.core.system.listReferences():
            self.colth.append(references_file(ref_obj))
        self.cam = camera()

    def replace_file(self):
        for obj in self.colth:
            obj.replace_file()
            self.qcolth_group.append(cloth_group_file(self.colth))

    def set_qcloth_attr(self):
        for obj in self.qcolth_group:
            obj.qcloth_update_pose()
            obj.set_cache_folder()

    def play_move(self):
        for obj in self.qcolth_group:
            obj.hide_other()
        self.cam.create_move()
        for obj in self.qcolth_group:
            obj.show_other()

    def export_fbx(self):
        for obj in self.qcolth_group:
            obj.export_fbx()

    def export_abc(self):
        for obj in self.qcolth_group:
            obj.export_abc()

    def save(self):
        pymel.core.system.saveAs("{}_sim_colth.ma".format(
            doodle_work_space.maya_file.name_not_ex))

    def __call__(self):
        self.replace_file()
        self.set_qcloth_attr()
        self.save()
        self.play_move()
        self.export_fbx()
        self.export_abc()
