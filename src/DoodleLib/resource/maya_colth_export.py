
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

pymel.core.system.openFile("D:/Autodesk/test/test2.ma",loadReferenceDepth="all")
pymel.core.playbackOptions(animationStartTime="950")
# 这个导出一定要在加载好场景后导入
import maya_fun_tool
reload(maya_fun_tool)
maya_fun_tool.cloth_export()()
