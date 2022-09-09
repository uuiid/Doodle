# -*- coding: utf-8 -*-
from maya import cmds
import maya.mel
import pymel.core.system
import pymel.util.path

import json


class maya_play_raneg():
    """
    maya文件范围
    """

    def __init__(self):
        self.start = int(cmds.playbackOptions(query=True, min=True))
        # self.start = 1000
        self.end = int(cmds.playbackOptions(query=True, max=True))

    def __str__(self):
        return "start: {}, end {}".format(self.start, self.end)


class maya_file():
    """
    maya文件本身的一些路径方便属性
    """

    def __init__(self):
        self._file_path = pymel.core.system.sceneName()
        self.maya_path_class = pymel.util.path(
            self._file_path)  # type: pymel.util.path
        self.file_name = pymel.util.path(
            self.maya_path_class.basename())  # type: pymel.util.path
        # type: pymel.util.path
        self.name_not_ex = self.file_name.splitext()[0]
        self.abs_path = self.maya_path_class.dirname()  # type: pymel.util.path

    def save(self):
        pass

    def __str__(self):
        return "maya file path : {} maya name :" \
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
        k_work = pymel.util.path(path).dirname() / \
                 "workspace.mel"  # type: pymel.util.path
        k_work2 = pymel.util.path(path) / "workspace.mel"

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
        # type: () -> pymel.util.path
        path = self.work.path / "abc" / \
               self.maya_file.name_not_ex  # type: pymel.util.path
        path.makedirs_p()
        return path

    def get_fbx_folder(self):
        # type: () -> pymel.util.path
        path = self.work.path / "fbx" / \
               self.maya_file.name_not_ex  # type: pymel.util.path
        path.makedirs_p()
        return path

    def get_move_folder(self):
        # type: () -> pymel.util.path
        path = self.work.path / "mov"  # type: pymel.util.path
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


class config(object):
    def __init__(self):
        self._path = pymel.util.path()
        self.export_path = pymel.util.path()
        self.project = pymel.util.path()
        self.t_post = 950
        self.export_anim_time = 1001

    @property
    def path(self):
        # type()->pymel.core.system.Path

        return self._path

    @path.setter
    def path(self, anys):
        self._path = pymel.util.path(anys)


class sim_config(config):
    def __init__(self):
        super(sim_config, self).__init__()
        self.only_sim = False
        self.upload_file = False
        self.export_fbx = True
        self.only_export = False

    def run(self):
        if self.only_export:
            self.only_sim = True
            self.config_file()
            self.export_file()
            if self.upload_file:
                self.update_file_fun()
            pass
        else:
            self.config_file()
            self.run_sim()
            self.export_file()
            if self.upload_file:
                self.update_file_fun()

    def config_file(self):

        cmds.doodle_load_project(project=self.project)

        cmds.doodle_create_ref_file()
        if not self.only_sim:
            cmds.doodle_ref_file_load()

    def run_sim(self):
        cmds.doodle_ref_file_sim(
            startTime=doodle_work_space.raneg.start,
            endTime=doodle_work_space.raneg.end)

        cmds.comm_play_blast_maya(startTime=self.export_anim_time,
                                  endTime=doodle_work_space.raneg.end,
                                  filepath="{path}/{base_name}_playblast_{start}-{end}.mp4"
                                  .format(
                                      path=doodle_work_space.get_move_folder(),
                                      base_name=doodle_work_space.maya_file.name_not_ex,
                                      start=self.export_anim_time,
                                      end=doodle_work_space.raneg.end
                                  ))

    def export_file(self):
        cmds.select(cl=True)

        if self.export_fbx:
            cmds.doodle_sequence_to_blend_shape_ref(
                startFrame=self.export_anim_time)
            cmds.doodle_ref_file_export(
                startTime=self.export_anim_time,
                endTime=doodle_work_space.raneg.end,
                exportType="fbx")
        else:
            cmds.doodle_ref_file_export(
                startTime=self.export_anim_time,
                endTime=doodle_work_space.raneg.end,
                exportType="abc")

    def update_file_fun(self):
        cmds.doodle_upload_files(clear=not self.cfg.upload_file)


class fbx_config(config):
    def __init__(self):
        super(fbx_config, self).__init__()
        self.use_all_ref = False
        self.upload_file = False


class replace_file(config):
    def __init__(self):
        super(replace_file, self).__init__()
        self.replace_file_all = False


def __load_config__(obj):
    l_config = None
    if "only_sim" in obj:
        l_config = sim_config()
        l_config.path = obj["path"]
        l_config.project = obj["project_"]
        l_config.only_sim = obj["only_sim"]
        l_config.upload_file = obj["upload_file"]
        l_config.export_fbx = obj["export_fbx"]
        l_config.only_export = obj["only_export"]

    elif "use_all_ref" in obj:
        l_config = fbx_config()
        l_config.path = obj["path"]
        l_config.project = obj["project_"]
        l_config.use_all_ref = obj["use_all_ref"]
        l_config.upload_file = obj["upload_file"]

    elif "replace_file_all" in obj:
        l_config = replace_file()
        l_config.path = obj["path"]
        l_config.project = obj["project_"]
        l_config.use_all_ref = obj["replace_file_all"]
    assert (l_config)

    l_config.export_anim_time = obj["export_anim_time"]
    l_config.t_post = obj["t_post"]
    return l_config


class open_file(object):
    maya_version = str(cmds.about(api=True))[0:4]
    doodle_plug = "doodle_maya_{}".format(str(cmds.about(api=True))[0:4])
    qcloth = "qualoth_{}_x64".format(str(cmds.about(api=True))[0:4])

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
        self.cfg = json.loads(in_str, object_hook=__load_config__)
        self.file_path = self.cfg.path

    def load_plug(self, str_list):
        # type: (list[str])->None
        # 这里加载一下我们自己的插件
        cmds.loadPlugin(open_file.doodle_plug)

        for plug in str_list:
            cmds.loadPlugin(plug)

    def open(self):
        maya_workspace.set_workspace_static(self.file_path.dirname())
        cmds.file(force=True, new=True)

        k_ref_ = None
        if isinstance(self.cfg, fbx_config) and self.cfg.use_all_ref:
            k_ref_ = "all"
        if k_ref_:
            cmds.file(self.file_path, open=True, loadReferenceDepth=k_ref_)

        else:
            cmds.file(self.file_path, open=True)

        if maya.mel.eval("currentTimeUnitToFPS") != 25.0:
            print("frame rate is not 25 is {}".format(
                maya.mel.eval("currentTimeUnitToFPS")
            ))
            quit()

        doodle_work_space.reset()
        doodle_work_space.set_workspace()

    def get_cloth_sim(self, qcloth_path=None):
        # type: (str) -> None

        self.load_plug(["AbcExport", "AbcImport", open_file.qcloth])
        self.open()
        cmds.playbackOptions(animationStartTime=950, minTime=950)
        cmds.currentTime(950)
        doodle_work_space.reset()

        assert (isinstance(self.cfg, sim_config))
        self.cfg.run()

    def get_fbx_export(self):
        # type: () -> None
        assert (isinstance(self.cfg, fbx_config))

        self.load_plug(["fbxmaya"])
        self.open()
        cmds.doodle_load_project(project=self.cfg.project)
        cmds.doodle_comm_file_save()
        cmds.doodle_create_ref_file()
        cmds.doodle_replace_rig_file()
        cmds.comm_play_blast_maya(startTime=doodle_work_space.raneg.start,
                                  endTime=doodle_work_space.raneg.end,
                                  filepath="{path}/{base_name}_playblast_{start}-{end}.mp4"
                                  .format(
                                      path=doodle_work_space.get_move_folder(),
                                      base_name=doodle_work_space.maya_file.name_not_ex,
                                      start=doodle_work_space.raneg.start,
                                      end=doodle_work_space.raneg.end
                                  ))
        cmds.doodle_ref_file_export(
            startTime=self.cfg.export_anim_time,
            endTime=doodle_work_space.raneg.end,
            exportType="fbx")
        cmds.doodle_export_camera(startTime=self.cfg.export_anim_time,
                                  endTime=doodle_work_space.raneg.end)
        cmds.doodle_upload_files(clear=not self.cfg.upload_file)

    def replace_file_fun(self):
        # type: () -> None
        assert (isinstance(self.cfg, replace_file))
        self.load_plug(["fbxmaya"])
        self.open()
        cmds.doodle_load_project(project=self.cfg.project)
        cmds.doodle_create_ref_file()
        cmds.doodle_replace_rig_file()
        cmds.doodle_comm_file_save()

    def __call__(self):
        if isinstance(self.cfg, sim_config):
            self.get_cloth_sim()
        elif isinstance(self.cfg, fbx_config):
            self.get_fbx_export()
        elif isinstance(self.cfg, replace_file):
            self.replace_file_fun()
        else:
            print("not config")
