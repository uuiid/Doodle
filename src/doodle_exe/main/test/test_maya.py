import maya.cmds
import sys
import os

curr_dir = os.path.dirname(os.path.realpath(__file__))
doodle_src = os.path.dirname(os.path.dirname(os.path.dirname(curr_dir)))
res_dir = doodle_src + """/doodle_lib/resource"""
sys.path.append(res_dir)
print(res_dir)

import maya_fun_tool


class doodle_main(object):
    maya_version = str(maya.cmds.about(api=True))[0:4]
    doodle_plug = "doodle_plug_{}".format(str(maya.cmds.about(api=True))[0:4])
    qcloth = "qualoth_{}_x64".format(str(maya.cmds.about(api=True))[0:4])

    def __init__(self):
        super().__init__()
        maya.cmds.loadPlugin(doodle_main.doodle_plug)
        maya.cmds.loadPlugin(doodle_main.qcloth)

    def open(self):
        pass


def doodle_test_sim():
    print("run test")
    k_f = maya_fun_tool.open_file()
    k_f.config_ = """
[
{"export_path":"",
"only_sim":false,
"path":"E:/tmp/cloth_test/TEST_EP001_SC001_AN.ma",
"qcloth_assets_path":"",
"project_":"E://tmp"
]"""
    k_f()


def doodle_test_fbx():
    print("run test")
    k_f = maya_fun_tool.open_file()
    k_f.config_ = """[{
        "export_path":"",
        "use_all_ref":false,
        "path":"E:/tmp/cloth_test/TEST_EP001_SC001_AN.ma",
        "project_":"E://tmp"
        }]"""
    k_f()


if __name__ == '__main__':
    doodle_test_sim()
    # doodle_test_fbx()
