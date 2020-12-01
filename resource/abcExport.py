# -*- coding: utf-8 -*-

import json
import re
import os
import maya.cmds
import pymel.core
import argparse
import maya.standalone

maya.standalone.initialize(name='python')


parser = argparse.ArgumentParser(description="exportMayaFile")
parser.add_argument("--name", "-n", help="name attr")
parser.add_argument("--path", "-p", help="path attr")
parser.add_argument("--exportpath", "-exp", help="export path attr")
parser.add_argument("--suffix", "-su", help="suffix", default=".ma")
args = parser.parse_args()


class doodle_log:
    def __init__(self):
        self.log = []
        self.logPath = args.exportpath

    def write(self):
        if not os.path.exists(self.logPath):
            os.makedirs(self.logPath)
        with open(os.path.join(self.logPath + "doodle_export_abc.json"), "w") as f:
            f.write(
                json.dumps(self.log,
                           ensure_ascii=False,
                           indent=4,
                           separators=(',', ':')
                           )
            )


DLOG = doodle_log()


class rootObj:

    # 初始化函数 获得根物体
    def __init__(self, py_obj):
        self.root = py_obj
        self.abcExportCom = \
            "AbcExport -j " \
            """"-frameRange {st} {end} """ \
            "-uvWrite " \
            "-writeFaceSets " \
            "-worldSpace " \
            "-dataFormat " \
            "ogawa " \
            "{mesh} " \
            """-file {pathFile}" """
        self.start = int(pymel.core.playbackOptions(query=True, min=True))
        self.end = int(pymel.core.playbackOptions(query=True, max=True))
        self.expotList = []
        self.mesh = None
        self.pathFile = None

    def __str__(self):
        return self.root.name().replace(":", "_")

    def name(self):
        return self.root.name().replace(":", "_")

    @staticmethod
    def seaneName():
        return os.path.basename(pymel.core.sceneName()).split(".")[0]

    # 可调用过滤器对象
    def filter(self, fun):
        return fun(self.root.name())

    # 传入导出路径并导出
    def export(self, path):
        self.pathFile = "{}/{}_{}.{}_{}.abc".format(path,
                                                    self.seaneName(),
                                                    self.name(),
                                                    self.start,
                                                    self.end)
        self._createMesh_()
        export_arg = self.abcExportCom.format(
            st=self.start,
            end=self.end,
            mesh=self.mesh,
            pathFile=os.path.abspath(self.pathFile).replace("\\", "/")
        )

        if self.expotList:
            print("abcexport {}".format(export_arg))
            DLOG.log.append({self.name(): self.pathFile,
                             "export_arg": export_arg})
            pymel.core.mel.eval(export_arg)

    def _createMesh_(self):
        pymel.core.select(self.root)
        # print(obj.listRelatives(ad=True, c=True, ni=True))
        # for i in obj.listRelatives(ad=True, c=True, ni=True):
        #     try:
        #         i.getShape()
        #     except:
        #         pass
        #     else:
        #         if i.getShape():
        #             if i.getShape().type() == "mesh":
        #                 print(i)
        k_lists = self.root.listRelatives(ad=True, c=True, ni=True)
        for k_list in k_lists:
            try:
                k_list.getShape()
            except:
                pass
            else:
                if k_list.getShape():
                    if k_list.getShape().type() == "mesh":
                        self.expotList.append(k_list)

        # self.expotList = pymel.core.ls(dag=True, ap=True, g=True, sl=True)
        for e_m_s in self.expotList:
            if self.mesh:
                self.mesh = "{} -root {}".format(self.mesh, e_m_s.longName())
            else:
                self.mesh = "-root {}".format(e_m_s.longName())
        if self.mesh:
            print("mesh root comm : {}".format(self.mesh))


def abc_xingling(name):
    return False if re.search("_Rig|Sim|nuc", name.split(":")[-1]) else True


def default_filter(maya_Obj):
    if maya_Obj.getShape():
        if maya_Obj.getShape().type() == "camera":
            return False
    return True


class run:
    def __init__(self):
        self.root_obj = []
        self.root_path = args.exportpath

    def __call__(self, *args, **kwargs):
        for r_e in pymel.core.ls(assemblies=True):
            r_obj = rootObj(r_e)
            if r_obj.filter(abc_xingling) and default_filter(r_e):
                print("get Root obj {}".format(r_e))
                self.root_obj.append(r_obj)
        if not os.path.exists(self.root_path):
            os.makedirs(self.root_path)

        for ex_obj in self.root_obj:
            print("export {}".format(ex_obj))
            ex_obj.export(self.root_path)

        # 导出日志
        DLOG.write()


pymel.core.newFile(force=True, type="mayaAscii")
pymel.core.openFile(os.path.join(
    args.path, args.name + args.suffix), force=True)

test = run()
test()
