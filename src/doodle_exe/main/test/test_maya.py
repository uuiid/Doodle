import maya.cmds
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


def doodle_test():
    print("run test")
    maya_fun_tool.open_file()
    raw_input("Please enter your name: ")
