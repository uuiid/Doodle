
# 一定要先初始化maya
import maya.standalone
maya.standalone.initialize(name='python')
# 然后是创建和打开物体
import pymel.core.system
import pymel.core
pymel.core.system.newFile(force=True)
pymel.core.system.loadPlugin("AbcExport")
pymel.core.system.loadPlugin("AbcImport")
pymel.core.system.loadPlugin("qualoth_2019_x64")

pymel.core.system.openFile(
    "D:/Autodesk/test/DBXY_163_54.ma", loadReferenceDepth="all")
if pymel.core.mel.eval("currentTimeUnitToFPS") != 25.0:
    pymel.core.warning("frame rate is not 25 is {}".format(
        pymel.core.mel.eval("currentTimeUnitToFPS")
        ))
    quit()
pymel.core.playbackOptions(animationStartTime="950")
# 这个导出一定要在加载好场景后导入
import maya_fun_tool
reload(maya_fun_tool)
maya_fun_tool.cloth_export("V:/03_Workflow/Assets/CFX/cloth")()
# maya_fun_tool.camera().create_move()
