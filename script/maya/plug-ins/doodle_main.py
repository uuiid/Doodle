import sys
import maya.api.OpenMaya as open_maya
import scripts.Doodle_shelf
import pymel.core


def maya_useNewAPI():
    """
    The presence of this function tells Maya that the plugin produces, and
    expects to be passed, objects created using the Maya Python API 2.0.
    """
    pass


# Initialize the plug-in
def initializePlugin(plugin):
    try:
        k_ver = str(pymel.versions.current())[0:4]
        pymel.core.loadPlugin("doodle_plug_{}".format(k_ver))
        scripts.Doodle_shelf.DoodleUIManage.creation()

    except:
        pass
        raise


# Uninitialize the plug-in
def uninitializePlugin(plugin):
    # pluginFn = OpenMayaMPx.MFnPlugin(plugin)
    try:
        k_ver = str(pymel.versions.current())[0:4]
        pymel.core.unloadPlugin("doodle_plug_{}".format(k_ver))
        scripts.Doodle_shelf.DoodleUIManage.deleteSelf()
        pass

    except:
        pass
        raise
