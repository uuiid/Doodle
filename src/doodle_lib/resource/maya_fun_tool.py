# -*- coding: utf-8 -*-
import pymel.core
import pymel.core.system
import pymel.core.nodetypes
import re
import json

import pymel.util.path
import pymel.all
import pymel.core.animation
# from pymel.core.system import FileReference
from pymel.core import FileReference


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
        k_work = pymel.core.Path(path).dirname() / \
            "workspace.mel"  # type: pymel.util.path
        k_work2 = pymel.core.Path(path) / "workspace.mel"

        if k_work.exists():
            pymel.core.system.workspace.open(k_work.dirname())
            pymel.core.system.workspace.fileRules["cache"] = "cache"
            pymel.core.system.workspace.fileRules["images"] = "images"
            pymel.core.system.workspace.fileRules["fileCache"] = "cache/nCache"
            return k_work.dirname()
        elif k_work2.exists():
            pymel.core.system.workspace.open(k_work2.dirname())
            pymel.core.system.workspace.fileRules["cache"] = "cache"
            pymel.core.system.workspace.fileRules["images"] = "images"
            pymel.core.system.workspace.fileRules["fileCache"] = "cache/nCache"
            return k_work2.dirname()
        else:
            pymel.core.system.workspace.open(k_work2.dirname())
            pymel.core.system.workspace.fileRules["cache"] = "cache"
            pymel.core.system.workspace.fileRules["images"] = "images"
            pymel.core.system.workspace.fileRules["fileCache"] = "cache/nCache"
            pymel.core.system.workspace.save()
            return k_work2.dirname()

    def __str__(self):
        return "{} ,{}".format(str(self.maya_file), str(self.raneg))

    def get_abc_folder(self):
        # type: () -> pymel.core.Path
        path = self.work.path / "abc" / self.maya_file.name_not_ex  # type: pymel.core.Path
        path.makedirs_p()
        return path

    def get_fbx_folder(self):
        # type: () -> pymel.core.Path
        path = self.work.path / "fbx" / self.maya_file.name_not_ex  # type: pymel.core.Path
        path.makedirs_p()
        return path


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
        self.loa = None
        self.priority_num = 0

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
            out_path = doodle_work_space.work.path / "mov"
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

        print("create move {}".format(out_path))
        try:
            pymel.core.comm_play_blast_maya(
                startTime=start_frame,
                endTime=end_frame,
                filepath="{path}/{base_name}_playblast_{start}-{end}.mp4"
                .format(
                    path=out_path,
                    base_name=doodle_work_space.maya_file.name_not_ex,
                    start=start_frame,
                    end=end_frame
                )
            )
        except AttributeError as err:
            print("arrt error use pymel.core.playblast")
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
            )

    def unlock_cam(self):
        for att in ["tx", "ty", "tz", "rx", "ry", "rz", "sx", "sy", "sz", "v", "coi", "sa", "fd", "fl", "vfa", "hfa", "lsr", "fs"]:
            if self.maya_cam.attr(att).isLocked():
                self.maya_cam.attr(att).unlock()

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
        print("unlock camera")
        self.unlock_cam()
        print("bake Results camera")
        pymel.core.bakeResults(self.maya_cam, sm=True,
                               t=(doodle_work_space.raneg.start,
                                  doodle_work_space.raneg.end))

        # FIXME 导出时值为1001
        mel_name = "{path}/{name}_camera_{start}-{end}.fbx".format(
            path=export_path,
            name=doodle_work_space.maya_file.name_not_ex,
            start=int(1001),
            end=int(doodle_work_space.raneg.end))
        mel_name = pymel.core.Path(mel_name)
        pymel.core.select(self.maya_cam)
        print("Prepare export path --> {}".format(mel_name))

        # pymel.core.mel.eval(
        #     "FBXExportBakeComplexStart -v {}".format(doodle_work_space.raneg.start))
        pymel.core.mel.eval(
            "FBXExportBakeComplexStart -v {}".format(1001))
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

        print("get cam {}.displayGateMaskOpacity attr".format(
            self.maya_cam.getShape().longName()))
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

    def __call__(self, in_path):
        self.export(in_path)

    def __str__(self):
        return "camera is {}, priority num is {}".format(
            self.maya_cam.fullPath(),
            self.priority_num)


class camera_filter:
    exclude = "(front|persp|side|top|camera)"

    def __init__(self):
        self.cam_list = []  # type:list[camera]
        for cam in pymel.core.ls(type='camera', l=True):
            l_c = camera()
            l_c.maya_cam = cam.getTransform()
            print("get {}".format(l_c))
            self.cam_list.append(l_c)

    def compute(self):
        for cam in self.cam_list:
            self._compute(cam=cam)
        self.cam_list.sort(key=lambda c: c.priority_num, reverse=True)
        print("--------------------- compute cam -----------------------")
        print("\n".join([str(i) for i in self.cam_list]))

    def _compute(self, cam):
        # type:(camera)->None
        # 匹配是否有集数和镜头标志
        if re.findall("""ep\d+_sc\d+""", cam.maya_cam.name(), re.I):
            cam.priority_num += 30

        if re.findall("""ep\d+""", cam.maya_cam.name(), re.I):
            cam.priority_num += 10

        if re.findall("""sc\d+""", cam.maya_cam.name(), re.I):
            cam.priority_num += 10

        if re.findall("""ep_\d+_sc_\d+""", cam.maya_cam.name(), re.I):
            cam.priority_num += 10

        if re.findall("""ep_\d+""", cam.maya_cam.name(), re.I):
            cam.priority_num += 5

        if re.findall("""sc_\d+""", cam.maya_cam.name(), re.I):
            cam.priority_num += 5

        # 匹配是否有大写开头的表示项目的字符
        if re.findall("""^[A-Z]+_""", cam.maya_cam.name(stripNamespace=True)):
            cam.priority_num += 2

        # 大致上匹配镜头和集数的
        if re.findall("""_\d+_\d+""", cam.maya_cam.name(stripNamespace=True)):
            cam.priority_num += 2

    def exclude_fun(self):
        for cam in self.cam_list:
            for test in filter(None, cam.maya_cam.fullPath().split("|")):
                if re.findall(self.exclude, test):
                    cam.priority_num = -1
                else:
                    cam.priority_num = 1
            if not re.findall("_", cam.maya_cam.name(stripNamespace=True)):
                cam.priority_num = -1
        self.old_cam = self.cam_list
        self.cam_list = filter(lambda c: c.priority_num >= 0, self.cam_list)
        print("--------------------- filter cam re -----------------------")
        print("\n".join([str(i) for i in self.cam_list]))

    def __call__(self):
        # type:()->camera
        self.exclude_fun()
        self.compute()
        if len(self.cam_list) > 0:
            return self.cam_list[0]
        else:
            return self.old_cam[0]


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
            print("not rename {}".format(str(self.name)))

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
        self.use_sim = False
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
        print("Speculation file is {}".format(self.cloth_path))
        # if self.path.fnmatch("*[rig].ma"):
        #     self.cloth_path = self.cfx_cloth_path / self.path.name.replace(
        #         "rig", "cloth")

    def select_to_ref(self, maya_obj):
        # type: (pymel.core.nodetypes.Transform)->bool
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

    @staticmethod
    def form_map(obj):
        # type: (dict[str,str])-> references_file
        maya_ref = pymel.core.FileReference(
            pathOrRefNode=pymel.core.Path(obj["path"]))
        k_ref = references_file(maya_ref)
        k_ref.use_sim = obj["use_sim"]
        return k_ref


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

        path = doodle_work_space.get_fbx_folder()   # type: pymel.core.Path

        name = "{}_{}_{}-{}.fbx".format(doodle_work_space.maya_file.name_not_ex,
                                        self.maya_name_space,
                                        doodle_work_space.raneg.start,
                                        doodle_work_space.raneg.end)
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
        # FIXME: 修复开始帧
        pymel.core.mel.eval(
            "FBXExportBakeComplexStart -v {}".format(1001))
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
            path = pymel.core.Path(
                "cache") / doodle_work_space.maya_file.name_not_ex / self.maya_name_space / select_str
            # 这里要删除缓存
            l_path_ = doodle_work_space.work.path / path  # type: pymel.core.Path
            if l_path_.exists():
                l_path_.rmtree()
                print("delete path {}".format(l_path_))

            doodle_work_space.work.mkdir(doodle_work_space.work.path / path)
            print(qcloth_obj)
            qcloth_obj.setAttr("cacheFolder", str(path))
            qcloth_obj.setAttr("cacheName", select_str)

    def create_cache(self):
        for obj in self.cloth_group:
            for m_attr in obj.getShapes():
                m_attr.outMesh.evaluate()

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
            export_path = doodle_work_space.work.path / "abc"
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
                print("{} not repair".format(str(obj)))
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
            export_path = doodle_work_space.get_abc_folder()
        path = export_path  # type: pymel.core.Path
        self.export_select_abc(
            path,
            pymel.core.ls(
                "{}:*UE4".format(self.maya_name_space),
                geometry=True,
                dagObjects=True),
            repeat)


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
        self.cam = camera_filter()()  # type: camera

    def export_fbx_mesh(self):
        self.cam(doodle_work_space.get_fbx_folder())
        self.cam.create_move(
            out_path=doodle_work_space.work.path / "mov",
            start_frame=1001)
        for obj in self.fbx_group:
            obj.export_fbx()

    def save(self):
        try:
            path = doodle_work_space.get_fbx_folder()  # type: pymel.core.util.path
            pymel.core.system.saveAs("{}/{}.ma".format(
                path,
                doodle_work_space.maya_file.name_not_ex))
        except RuntimeError as err:
            print("not save file")

    def __call__(self):
        self.save()
        self.export_fbx_mesh()


class cloth_export():
    def __init__(self, cfx_path):
        pymel.core.animation.evaluationManager(mode="off")
        doodle_work_space.set_workspace()
        print(doodle_work_space)

        references_file.cfx_cloth_path = pymel.core.Path(cfx_path)

        self.colth_ref = []  # type: list[references_file]
        self.qcolth_group = []  # type: list[cloth_group_file]
        self.cam = camera_filter()()  # 调整camera的位置， 保证在替换引用之前找到cam
        self.select_sim_references_file()
        self.qcolth_group = [cloth_group_file(
            i) for i in self.colth_ref if i.is_valid()]

    def select_sim_references_file(self):
        try:
            meta = pymel.core.modeling.getMetadata(
                channelName="doodle_sim_json", streamName="json_stream", memberName="json", scene=True, index="0")
            if meta:
                obj_dirt = json.loads(meta[0])
                k_colth_ref = []  # type: list[references_file]
                for i in obj_dirt:
                    k_colth_ref.append(references_file.form_map(i))
                self.colth_ref = [i for i in k_colth_ref if i.use_sim]

            else:
                self.colth_ref = [references_file(i)
                                  for i in pymel.core.listReferences()]
        except RuntimeError:
            self.colth_ref = [references_file(i)
                              for i in pymel.core.listReferences()]

    def replace_file(self):
        for qc in self.qcolth_group:
            qc.maya_ref.replace_file()
            qc.reset(qc.maya_ref)

    def set_qcloth_attr(self):
        for obj in self.qcolth_group:
            # obj.qcloth_update_pose()
            obj.set_cache_folder()

    def play_move(self):
        global doodle_work_space
        print("create move {} to {}".format(
            1001,
            doodle_work_space.raneg.end))
        self.cam.create_move(
            out_path=doodle_work_space.work.path / "mov",
            start_frame=1001
        )

    def export_fbx(self):
        for obj in self.qcolth_group:
            obj.export_fbx()

    def creare_cache(self):
        for i in range(doodle_work_space.raneg.start, doodle_work_space.raneg.end):
            pymel.core.currentTime(i)
            for obj in self.qcolth_group:
                obj.create_cache()

    def export_abc(self):
        for obj in self.qcolth_group:
            obj.export_abc(repeat=False)

    def save(self, override=False):
        try:
            if not override:
                path = doodle_work_space.work.path / "cloth_ma"  # type: pymel.core.util.path
                path.makedirs_p()

                pymel.core.system.saveAs("{}/{}_sim_colth.ma".format(
                    path,
                    doodle_work_space.maya_file.name_not_ex))
            else:
                pymel.core.system.saveFile(force=True)

        except RuntimeError as err:
            print("not save file ")

    def sim_and_export(self):
        self.set_qcloth_attr()
        self.save(override=True)
        self.creare_cache()
        self.play_move()
        self.export_abc()

        # self.export_fbx()

    def __call__(self):
        self.replace_file()
        self.set_qcloth_attr()
        self.save()
        self.creare_cache()
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
            print("not get episodes and shots")

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


class config(object):
    def __init__(self):
        self._path = pymel.core.Path()
        self.export_path = pymel.core.Path()

    @property
    def path(self):
        # type()->pymel.core.system.Path

        return self._path

    @path.setter
    def path(self, anys):
        self._path = pymel.core.Path(anys)


class sim_config(config):
    def __init__(self):
        super(sim_config, self).__init__()
        self.qcloth_assets_path = pymel.core.Path()
        self.only_sim = False


class fbx_config(config):
    def __init__(self):
        super(fbx_config, self).__init__()
        self.use_all_ref = False


def __load_config__(obj):
    if "qcloth_assets_path" in obj:
        k_con = sim_config()
        k_con.path = obj["path"]
        k_con.qcloth_assets_path = obj["qcloth_assets_path"]
        k_con.export_path = obj["export_path"]
        k_con.only_sim = obj["only_sim"]
        return k_con
    elif "use_all_ref" in obj:
        k_con = fbx_config()
        k_con.path = obj["path"]
        k_con.export_path = obj["export_path"]
        k_con.use_all_ref = obj["use_all_ref"]
        return k_con


class open_file(object):
    maya_version = str(pymel.versions.current())[0:4]
    doodle_plug = "doodle_plug_{}".format(str(pymel.versions.current())[0:4])
    qcloth = "qualoth_{}_x64".format(str(pymel.versions.current())[0:4])

    def __init__(self, in_config=None):
        # type:(str,config)->None
        self.cfg = in_config  # type: config
        self.file_path = None
        if self.cfg:
            self.file_path = self.cfg.path

    @property
    def config_(self):
        return self.cfg

    @config_.setter
    def config_(self, in_str):
        self.cfg = json.loads(in_str, object_hook=__load_config__)[0]
        self.file_path = self.cfg.path

    def load_plug(self, str_list):
        # type: (list[str])->None
        try:
            # 这里加载一下我们自己的插件
            pymel.core.system.loadPlugin(open_file.doodle_plug)
        except RuntimeError as err:
            # print(err)
            print("not load {}".format(open_file.doodle_plug))

        for plug in str_list:
            pymel.core.system.loadPlugin(plug)

    def open(self):
        maya_workspace.set_workspace_static(self.file_path.dirname())
        pymel.core.system.newFile(force=True)

        k_ref_ = None
        if isinstance(self.cfg, fbx_config) and self.cfg.use_all_ref:
            k_ref_ = "all"
        if k_ref_:
            pymel.core.system.openFile(
                self.file_path, loadReferenceDepth=k_ref_)
        else:
            pymel.core.system.openFile(
                self.file_path)

        if pymel.core.mel.eval("currentTimeUnitToFPS") != 25.0:
            print("frame rate is not 25 is {}".format(
                pymel.core.mel.eval("currentTimeUnitToFPS")
            ))
            quit()

        doodle_work_space.reset()
        doodle_work_space.set_workspace()

    def get_cloth_sim(self, qcloth_path=None):
        # type: (str) -> cloth_export

        self.load_plug(["AbcExport", "AbcImport", open_file.qcloth])
        self.open()
        pymel.core.playbackOptions(animationStartTime=950, min=950)
        pymel.core.currentTime(950)
        doodle_work_space.reset()

        assert(isinstance(self.cfg, sim_config))
        qcloth_path = pymel.core.Path(self.cfg.qcloth_assets_path)
        k_cl = cloth_export(qcloth_path)
        if self.cfg.only_sim:
            k_cl.sim_and_export()
        else:
            k_cl()

    def get_fbx_export(self):
        # type: () -> fbx_export
        assert(isinstance(self.cfg, fbx_config))

        self.load_plug(["fbxmaya"])
        self.open()
        fbx_export()()

    def __call__(self):
        if isinstance(self.cfg, sim_config):
            self.get_cloth_sim()
        elif isinstance(self.cfg, fbx_config):
            self.get_fbx_export()
        else:
            print("not config")
