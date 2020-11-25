import sys
import maya.api.OpenMaya as om
import maya.OpenMayaMPx as OpenMayaMPx
import scripts.Doodle_shelf

#
# def maya_useNewAPI():
#     """
#     The presence of this function tells Maya that the plugin produces, and
#     expects to be passed, objects created using the Maya Python API 2.0.
#     """
#     pass


# command
class PyHelloWorldCmd(OpenMayaMPx.MPxCommand):

    kPluginCmdName = "Doodle_main"

    def __init__(self):
        OpenMayaMPx.MPxCommand.__init__(self)

    @staticmethod
    def cmdCreator():
        return PyHelloWorldCmd()

    def doIt(self, args):
        print("load")


# Initialize the plug-in
def initializePlugin(plugin):
    pluginFn = OpenMayaMPx.MFnPlugin(plugin, "doodle", "1.0.0")
    dle_plugin_ui = OpenMayaMPx.MFnPlugin(plugin, "doodleUI", "1.0.0")
    try:
        pluginFn.registerCommand(
            PyHelloWorldCmd.kPluginCmdName, PyHelloWorldCmd.cmdCreator
        )
        dle_plugin_ui.registerUI(
            scripts.Doodle_shelf.DoodleUIManage.creation, scripts.Doodle_shelf.DoodleUIManage.deleteSelf
        )
    except:
        sys.stderr.write(
            "Failed to register command: %s\n" % PyHelloWorldCmd.kPluginCmdName
        )
        raise


# Uninitialize the plug-in
def uninitializePlugin(plugin):
    pluginFn = OpenMayaMPx.MFnPlugin(plugin)
    try:
        pluginFn.deregisterCommand(PyHelloWorldCmd.kPluginCmdName)
    except:
        sys.stderr.write(
            "Failed to unregister command: %s\n" % PyHelloWorldCmd.kPluginCmdName
        )
        raise
