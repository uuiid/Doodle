# -*- coding: utf-8 -*-
import pymel.core
import pymel.core.system
import pymel.core.nodetypes
import argparse
import os
import re
import json

import pymel.util.path
import pymel.all
import pymel.core.animation


class maya_play_raneg():
    """
    maya文件范围
    """

    def __init__(self):
        self.start = int(pymel.core.playbackOptions(query=True, min=True))
        # self.start = 1000
        self.end = int(pymel.core.playbackOptions(query=True, max=True))

    def __str__(self):
        return "start: {}, end {}".format(self.start, self.end)


class maya_file():
    """
    maya文件本身的一些路径方便属性
    """

    def __init__(self):
        self._file_path = pymel.core.system.sceneName()
        self.maya_path_class = pymel.core.Path(
            self._file_path)  # type: pymel.core.Path
        self.file_name = pymel.core.Path(
            self.maya_path_class.basename())  # type: pymel.core.Path
        # type: pymel.core.Path
        self.name_not_ex = self.file_name.splitext()[0]
        self.abs_path = self.maya_path_class.dirname()  # type: pymel.core.Path

    def save_as_cloth(self):
        pass

    def __str__(self):
        return "maya file path : {} maya name :"\
              .format(self.abs_path,
                      self.file_name)


class maya_workspace():
    """
    maya 工作空间的一些创建和使用
    """

    def __init__(self):
        self.work = pymel.core.system.workspace
        self.work.update()
        self.maya_file = maya_file()
        self.raneg = maya_play_raneg()

    def set_workspace(self):
        self.work.open(maya_workspace.set_workspace_static(
            self.maya_file.abs_path))

    def reset(self):
        self.work = pymel.core.system.workspace
        self.work.update()
        self.maya_file = maya_file()
        self.raneg = maya_play_raneg()

    @staticmethod
    def set_workspace_static(path):
        # type: (str)->pymel.util.path
        k_work = pymel.core.Path(path).dirname() / "workspace.mel" # type: pymel.util.path
        k_work2 = pymel.core.Path(path) / "workspace.mel"

        if k_work.exists():
            pymel.core.system.workspace.open(k_work.dirname())
            return k_work.dirname()
        elif k_work2.exists():
            pymel.core.system.workspace.open(k_work2.dirname())
            return k_work2.dirname()
        else:
            pymel.core.system.workspace.open(k_work2.dirname())
            pymel.core.system.workspace.fileRules["cache"] = "cache"
            pymel.core.system.workspace.fileRules["images"] = "images"
            pymel.core.system.workspace.save()
            return k_work2.dirname()

    def __str__(self):
        return "{} ,{}".format(str(self.maya_file), str(self.raneg))


class export_log(object):
    """
    导出问价使用的log文件类
    """

    def addfile(self, objname, path, version):
        setattr(self, objname, [path, version])


# 全局名称空间变量，
doodle_work_space = maya_workspace()
# 全局log变量
log = export_log()


class camera:
    """
    代表maya文件中的cam
    这个选择时会排除掉默认的和不加下滑线的cam
    """

    exclude = "(front|persp|side|top|camera)"

    def __init__(self):
        self.maya_cam = None  # type: pymel.core.nodetypes.Transform
        self.filter_cam()
        self.loa = None

    def filter_cam(self):
        # 过滤cam
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
                return
        print("not select cam ", self.maya_cam)

    def create_move(self, out_path=None,
                    start_frame=None,
                    end_frame=None):
        if not start_frame:
            start_frame = doodle_work_space.raneg.start
        if not end_frame:
            end_frame = doodle_work_space.raneg.end
        # 创建视频
        # 如果不符合就直接返回
        if not self.maya_cam:
            return
        if not out_path:
            out_path = doodle_work_space.maya_file.abs_path / "mov"
        tmp_path = pymel.core.Path(out_path)
        if not tmp_path.exists():
            tmp_path.makedirs_p()

        render_node = pymel.core.ls("defaultRenderGlobals")[0]

        render_node.imageFormat.set(8)
        render_node.animation.set(True)
        render_node.putFrameBeforeExt.set(True)
        render_node.useFrameExt.set(False)
        render_node.useMayaFileName.set(True)
        render_node.imageFilePrefix.set("<Scene>/<Scene>")
        render_node.startFrame.set(doodle_work_space.raneg.start)
        render_node.endFrame.set(doodle_work_space.raneg.end)
        Resolution_node = pymel.core.ls("defaultResolution")[0]
        Resolution_node.height.set(1920)
        Resolution_node.width.set(1280)

        for cam in pymel.core.listCameras():
            pymel.core.ls(cam)[0].renderable.set(False)
        self.maya_cam.renderable.set(True)
        # print("maya mel eval lookThroughModelPanel {} modelPanel1;".format(self.maya_cam.fullPath()))
        # pymel.core.mel.eval(
        #     "lookThroughModelPanel {} modelPanel1;".format(self.maya_cam.fullPath()))
        # try:
        #     # type: pymel.core.nodetypes.RenderGlobals
        #     # pymel.core.hwRender(camera=self.maya_cam, h=1280, w=1920)
        #     pymel.core.playblast(
        #         viewer=False,
        #         startTime=start_frame,
        #         endTime=end_frame,
        #         filename="{path}/{base_name}_playblast_{start}-{end}"
        #         .format(
        #             path=out_path,
        #             base_name=doodle_work_space.maya_file.name_not_ex,
        #             start=start_frame,
        #             end=end_frame
        #         ),
        #         percent=100,
        #         quality=100,
        #         # offScreen=True,
        #         # editorPanelName="modelPanel1",
        #         format="qt",
        #         compression="H.264",
        #         widthHeight=(1920, 1280)
        #     )
        # except RuntimeError:
        pymel.core.system.warning("QuickTime not found, use default value")
        print("create move {}".format(out_path))
        pymel.core.playblast(
            viewer=False,
            startTime=start_frame,
            endTime=end_frame,
            filename="{path}/{base_name}_playblast_{start}-{end}"
            .format(
                path=out_path,
                base_name=doodle_work_space.maya_file.name_not_ex,
                start=start_frame,
                end=end_frame
            ),
            percent=100,
            quality=100,
            widthHeight=(1920, 1280)
            # editorPanelName="modelPanel4"
            # offScreen=True
        )
        print("create move {}".format(out_path))

    def export(self, export_path):

        # 如果不符合就直接返回
        if not self.maya_cam:
            return
        tmp_path = pymel.core.Path(export_path)
        if not tmp_path.exists():
            tmp_path.makedirs_p()

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
        mel_name = pymel.core.Path(mel_name)
        pymel.core.select(self.maya_cam)
        print("Prepare export path ", mel_name)

        pymel.core.mel.eval(
            "FBXExportBakeComplexStart -v {}".format(doodle_work_space.raneg.start))
        pymel.core.mel.eval(
            "FBXExportBakeComplexEnd -v {}".format(doodle_work_space.raneg.end))
        pymel.core.mel.eval("FBXExportBakeComplexAnimation -v true")
        pymel.core.mel.eval("FBXExportConstraints -v true")
        pymel.core.mel.eval(
            'FBXExport -f "{}" -s'.format(str(mel_name.abspath()).replace("\\", "/")))
        print("camera erport ----> {}".format(str(mel_name.abspath()).replace("\\", "/")))
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
            pymel.core.mel.eval('setAttr "{}.displayGateMaskOpacity" 1;'.format(
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
            str = doodle_work_space.maya_file.abs_path / \
                doodle_work_space.maya_file.name_not_ex / "fbx"
        self.export(str)
        try:
            pass
        except:
            print("not export")


class meateral():
    def __init__(self, meateral_obj):
        self.maya_obj = meateral_obj
        self.name = self.maya_obj.name()
        self.shader_group = None
        self.shader = None
        grp = self.maya_obj.shadingGroups()
        if grp:
            self.shader_group = grp[0]
            self.shader = self.shader_group.name()

    def chick(self):
        self.reName()

    def reName(self):
        try:
            if self.maya_obj and self.shader_group and not re.search("Mat$", self.name):
                self.maya_obj.rename("{}Mat".format(self.name))
                self.shader_group.rename(self.name)
                print("rename to {} ".format(self.name))
            else:
                print("not rename {}".format(self.maya_obj))
        except RuntimeError:
            pymel.core.warning("not rename {}".format(str(self.name)))

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

    def __eq__(self, o):
        # type : (meateral)->bool
        return o.shader_group == self.shader_group

    def __hash__(self):
        # type : ()->int
        return self.shader_group.__hash__


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
        self.maya_mesh_obj = None  # type: pymel.core.nodetypes.Shape
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
    cfx_cloth_path = pymel.core.Path()  # tyep: type: pymel.core.Path

    def __init__(self, ref_obj):
        # type:(pymel.core.FileReference)->None

        self.maya_ref = ref_obj
        self.namespace = None  # type:str
        self.cloth_path = None  # type: pymel.core.Path
        if self.maya_ref:
            self._set_init_()

    ##
    # 创建colth 文件路径
    def _set_init_(self):
        self.path = pymel.core.Path(
            self.maya_ref.path)  # type: pymel.core.Path
        self.namespace = self.maya_ref.fullNamespace
        self.cloth_path = pymel.core.Path()
        self.cloth_path = self.cfx_cloth_path / \
            "{}_cloth{}".format(self.path.namebase, self.path.ext)
        print("replace_file {}".format(self.cloth_path))
        # if self.path.fnmatch("*[rig].ma"):
        #     self.cloth_path = self.cfx_cloth_path / self.path.name.replace(
        #         "rig", "cloth")

    def select_to_ref(self, maya_obj):
        # type: (Any)->bool
        try:
            ref = pymel.core.referenceQuery(
                maya_obj, referenceNode=True, topReference=True)
            if ref:
                self.replace_file = pymel.core.FileReference(ref)
                self._set_init_()
                return True
            else:
                return False
        except RuntimeError:
            return False

    def isLoaded(self):
        return self.maya_ref.isLoaded()

    def is_valid(self):
        # type:()->bool
        is_colth = re.findall("_cloth$", self.path.namebase)
        colth_ex = self.cloth_path.exists()
        return self.isLoaded() and (is_colth or colth_ex)
    ##
    # 如果存在就替换路径

    def replace_file(self):
        # type:()->bool
        if(self.cloth_path.exists()):
            self.maya_ref.replaceWith(self.cloth_path)
            self.namespace = self.maya_ref.namespace
            return True
        return False

    def importContents(self):
        if self.maya_ref:
            self.maya_ref.importContents()


class export_group(object):
    def __init__(self, ref_obj):
        self.reset(ref_obj)

    def reset(self, ref_obj):
        # type:(references_file)->None
        self.maya_name_space = ref_obj.namespace

    def export_fbx(self, select_str):
        mesh = pymel.core.ls(select_str)
        pymel.core.select(mesh)

        if not mesh:
            return

        path = doodle_work_space.work.getPath() \
            / doodle_work_space.maya_file.name_not_ex / "fbx"  # type: pymel.core.Path

        name = "{}_{}_{}-{}.fbx".format(doodle_work_space.maya_file.name_not_ex,
                                        self.maya_name_space,
                                        doodle_work_space.raneg.start,
                                        doodle_work_space.raneg.end)
        path.makedirs_p()
        path = path / name
        print("export path : {}".format(path.abspath()).replace("\\", "/"))
        pymel.core.bakeResults(simulation=True,
                               time=(doodle_work_space.raneg.start,
                                     doodle_work_space.raneg.end),
                               hierarchy="below",
                               sampleBy=1,
                               disableImplicitControl=True,
                               preserveOutsideKeys=False,
                               sparseAnimCurveBake=False)
        pymel.core.mel.eval(
            "FBXExportBakeComplexStart -v {}".format(doodle_work_space.raneg.start))
        pymel.core.mel.eval(
            "FBXExportBakeComplexEnd -v {}".format(doodle_work_space.raneg.end))
        pymel.core.mel.eval("FBXExportBakeComplexAnimation -v true")
        pymel.core.mel.eval("FBXExportSmoothingGroups -v true")
        pymel.core.mel.eval("FBXExportConstraints -v true")
        pymel.core.mel.eval(
            'FBXExport -f "{}" -s'.format(path.abspath()).replace("\\", "/"))


class cloth_group_file(export_group):
    def __init__(self, ref_obj):
        # type:(references_file)->None
        super(cloth_group_file, self).__init__(ref_obj)
        self.reset(ref_obj)

    def reset(self, ref_obj):
        # type:(references_file)->None
        super(cloth_group_file, self).reset(ref_obj)
        self.maya_ref = ref_obj
        pymel.core.select(clear=True)
        self.cloth_group = pymel.core.ls(
            "{}:*cloth_proxy".format(self.maya_name_space))

        # type: list[pymel.core.nodetypes.Transform]
        self.maya_abc_export = None

    def qcloth_update_pose(self):
        for obj in self.cloth_group:
            select_str = obj.name(stripNamespace=True).replace(
                "cloth_proxy", "skin_proxy")
            pymel.core.select(
                "{}:*{}".format(self.maya_name_space, select_str), replace=True)
            pymel.core.select(obj, add=True)
            print("select {} and {}".format(str(select_str), str(obj)))
            print(pymel.core.selected())
            pymel.core.mel.eval("qlUpdateInitialPose;")

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
        pymel.core.select("{}:*UE4".format(self.maya_name_space))
        pymel.core.hide(pymel.core.selected())

    def show_other(self):
        pymel.core.select("{}:*UE4".format(self.maya_name_space))
        pymel.core.showHidden(pymel.core.selected())

    def export_fbx(self):
        super(cloth_group_file, self).export_fbx(
            "{}:*UE4".format(self.maya_name_space))

    def export_select_abc(
            self,
            export_path=pymel.core.Path(),
            select_obj=[],
            repeat=False):
        # type: (pymel.core.Path,list[pymel.core.nodetypes.Transform],bool)->None

        # 选择物体导入
        self.maya_ref.importContents()

        # 创建路径
        if not export_path:
            export_path = doodle_work_space.work.getPath() / "abc"
        path = export_path  # type: pymel.core.Path

        path.makedirs_p()

        # 导出物体
        # list[pymel.core.nodetypes.Transform]
        self.maya_abc_export = select_obj

        if not self.maya_abc_export:
            return

        # 先找到的所有集合体
        # geo_group = []  # type: list[geometryInfo]
        for obj in self.maya_abc_export:
            try:
                k_geo = geometryInfo(obj)
                k_geo.repair()
            except IndexError:
                pymel.core.warning("{} not repair".format(str(obj)))
            # geo_group.append(geometryInfo(obj))

        # mat_group = []  # type: list[meateral]
        # mat_node_set = set()
        # for geo in geo_group:  # 获得所有几何体中的mat
        #     for mat in geo.materals:  # 获得mat 中的mat maya node 然后插入集合去重
        #         mat_node_set.add(mat.maya_obj)
        # for mat in mat_node_set:  # 去重后的集合体重命名
        #     meateral(mat).reName()

        export_abc = None  # type: pymel.core.nodetypes.Transform
        if len(self.maya_abc_export) > 1:
            export_abc = pymel.core.polyUnite(self.maya_abc_export)[0]
        else:
            export_abc = self.maya_abc_export[0]

        try:
            export_abc = export_abc.getTransform()
        except AttributeError:
            pass

        abcexmashs = "-root {}".format(export_abc.fullPath())
        # abcexmashs = ""
        # for exmash in self.export_abc:
        #     abcexmashs = "{} -root {}".format(abcexmashs,
        #                                       exmash.fullPathName())
        # -stripNamespaces
        if repeat:
            name = "{}_{}_{}-{}.abc".format(doodle_work_space.maya_file.name_not_ex,
                                            self.maya_name_space,
                                            1000,
                                            doodle_work_space.raneg.end)
            k_path = path / name
            print("export path : {}".format(k_path))
            abcExportCom = """AbcExport -j "-frameRange {f1} {f2} -stripNamespaces -uvWrite -writeFaceSets -worldSpace -dataFormat ogawa {mash} -file {f0}" """ \
                .format(f0=str(k_path.abspath()).replace("\\", "/"),
                        f1=doodle_work_space.raneg.start, f2=doodle_work_space.raneg.end,
                        mash=abcexmashs)
            print(abcExportCom)
            pymel.core.mel.eval(abcExportCom)

        name = "{}_{}_{}-{}.abc".format(doodle_work_space.maya_file.name_not_ex,
                                        self.maya_name_space,
                                        1000,
                                        doodle_work_space.raneg.end)
        path = path / name
        print("export path : {}".format(path))
        abcExportCom = """AbcExport -j "-frameRange {f1} {f2} -stripNamespaces -uvWrite -writeFaceSets -worldSpace -dataFormat ogawa {mash} -file {f0}" """ \
            .format(f0=str(path.abspath()).replace("\\", "/"),
                    f1=1000, f2=doodle_work_space.raneg.end,
                    mash=abcexmashs)
        print(abcExportCom)
        pymel.core.mel.eval(abcExportCom)

    def export_abc(self, export_path=pymel.core.Path(), repeat=False):
        # 创建路径
        if not export_path:
            export_path = doodle_work_space.work.getPath() / "abc"
        path = export_path  # type: pymel.core.Path
        self.export_select_abc(
            path,
            pymel.core.ls(
                "{}:*UE4".format(self.maya_name_space),
                geometry=True,
                dagObjects=True),
            repeat)

    def dgeval(self):
        for obj in pymel.core.ls("{}:*_cloth".format(self.maya_name_space)):
            print("dgeval {}".format(str(obj.getShapes()[0].outputMesh)))
            pymel.core.system.dgeval(obj.getShapes()[0].outputMesh)


class fbx_group_file(export_group):
    def __init__(self, ref_obj):
        # type:(references_file)->None
        super(fbx_group_file, self).__init__(ref_obj)

    def export_fbx(self):
        return super(fbx_group_file, self).export_fbx("{}:*UE4".format(self.maya_name_space))


class fbx_export():
    def __init__(self):
        doodle_work_space.set_workspace()
        self.ref = []  # type: list[references_file]
        self.fbx_group = []  # type: list[fbx_group_file]
        for ref_obj in pymel.core.system.listReferences():
            k_ref = references_file(ref_obj=ref_obj)
            if k_ref.isLoaded():
                self.ref.append(k_ref)
        for ref_obj in self.ref:
            self.fbx_group.append(fbx_group_file(ref_obj))
        self.cam = camera()

    def export_fbx_mesh(self):
        self.cam()
        for obj in self.fbx_group:
            obj.export_fbx()

    def __call__(self):
        self.export_fbx_mesh()


class cloth_export():
    def __init__(self, cfx_path):
        pymel.core.animation.evaluationManager(mode="off")
        doodle_work_space.set_workspace()
        print(doodle_work_space)

        references_file.cfx_cloth_path = pymel.core.Path(cfx_path)

        self.colth_ref = []  # type: list[references_file]
        self.qcolth_group = []  # type: list[cloth_group_file]
        self.cam = camera()  # 调整camera的位置， 保证在替换引用之前找到cam
        self.select_sim_references_file()
        self.qcolth_group = [cloth_group_file(
            i) for i in self.colth_ref if i.is_valid()]

    def select_sim_references_file(self):
        if pymel.core.fileInfo.has_key("doodle_sim"):
            k_set = eval(pymel.core.fileInfo["doodle_sim"])
            self.colth_ref = [references_file(i) for i in k_set]
        else:
            self.colth_ref = [references_file(i)
                              for i in pymel.core.listReferences()]

    def replace_file(self):
        for obj in self.colth_ref:
            obj.replace_file()

        for qc in self.qcolth_group:
            qc.reset(qc.maya_ref)

    def set_qcloth_attr(self):
        for obj in self.qcolth_group:
            # obj.qcloth_update_pose()
            obj.set_cache_folder()

    def play_move(self):
        global doodle_work_space
        print("create move {} to {}".format(
            doodle_work_space.raneg.start,
            doodle_work_space.raneg.end))
        self.cam.create_move()
        self.cam.create_move(
            out_path=doodle_work_space.maya_file.abs_path / "mov",
            start_frame=1001
        )
        # pymel.core.playbackOptions(maxPlaybackSpeed=0,blockingAnim=True,view="all",loop="once")
        # pymel.core.play(forward=True)
        # pymel.core.select(clear=True)
        # for f in range(
        #     doodle_work_space.raneg.start,
        #     doodle_work_space.raneg.end,
        #         1):
        #     pymel.core.currentTime(f, update= False)
        #     for obj in self.qcolth_group:
        #         obj.dgeval()
        # self.cam.create_move()

    def export_fbx(self):
        for obj in self.qcolth_group:
            obj.export_fbx()

    def export_abc(self):
        for obj in self.qcolth_group:
            obj.export_abc(repeat=True)

    def save(self, override=False):
        if not override:
            path = doodle_work_space.maya_file.abs_path / \
                doodle_work_space.maya_file.name_not_ex  # type: pymel.core.Path
            path.makedirs_p()

            pymel.core.system.saveAs("{}/{}_sim_colth.ma".format(
                path,
                doodle_work_space.maya_file.name_not_ex))
        else:
            pymel.core.system.saveFile(force=True)

    def sim_and_export(self):
        self.set_qcloth_attr()
        self.save(override=True)
        if len(self.qcolth_group) > 5:
            self.export_abc()
            self.play_move()
        else:
            self.play_move()
            self.export_abc()
        # self.export_fbx()

    def __call__(self):
        self.replace_file()
        self.set_qcloth_attr()
        self.save()
        if len(self.qcolth_group) > 5:
            self.export_abc()
            self.play_move()
        else:
            self.play_move()
            self.export_abc()


class analyseFileName():

    def __init__(self):
        self.filename = str(doodle_work_space.maya_file.name_not_ex)
        self.eps = None
        self.shot = None
        self.shot_ab = ""

    def analyse(self):
        name_parsing_ep = re.findall("ep(\d+)", self.filename)
        name_parsing_shot = re.findall("sc(\d+)([_|a-z])", self.filename)
        if name_parsing_ep and name_parsing_shot:
            try:
                self.eps = int(name_parsing_ep[0])
                self.shot = int(name_parsing_shot[0][0])
                shotab = name_parsing_shot[0][1]
                if shotab != "_":
                    self.shotab = shotab
            except NameError:
                print("not get episodes and shots")
        else:
            pymel.core.warning("not get episodes and shots")

    def path(self):
        # type:()->pymel.core.Path
        self.analyse()
        path = None
        if self.eps and self.shot:
            path = "/03_Workflow/shots/ep{eps:0>3d}/sc{shot:0>4d}{shotab}/".format(
                eps=self.eps,
                shot=self.shot,
                shotab=self.shot_ab
            )
        else:
            path = "/03_Workflow/shots/"

        return pymel.core.Path(path)


class open_file():
    def __init__(self, file_path):
        self.file_path = pymel.core.Path(file_path)

    def load_plug(self):
        pymel.core.system.loadPlugin("AbcExport")
        pymel.core.system.loadPlugin("AbcImport")
        pymel.core.system.loadPlugin("qualoth_2019_x64")

    def open(self):
        self.load_plug()
        maya_workspace.set_workspace_static(self.file_path.dirname())

        pymel.core.system.newFile(force=True)
        pymel.core.system.openFile(self.file_path, loadReferenceDepth="all")
        if pymel.core.mel.eval("currentTimeUnitToFPS") != 25.0:
            pymel.core.warning("frame rate is not 25 is {}".format(
                pymel.core.mel.eval("currentTimeUnitToFPS")
            ))
            quit()

        global doodle_work_space
        doodle_work_space.reset()
        doodle_work_space.set_workspace()

    def get_cloth_sim(self, qcloth_path):
        # type: (str)->cloth_export
        self.open()
        pymel.core.playbackOptions(animationStartTime="950")
        return cloth_export(qcloth_path)

    def get_fbx_export(self):
        # type :(str)->fbx_export
        self.open()
        return fbx_export()
