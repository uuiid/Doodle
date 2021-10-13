#include <MotionMayaPlugInit.h>

#include <maya/MFnPlugin.h>

MStatus initializePlugin(MObject obj) {
  /**
   * @brief 添加插件注册方法
   */
  MStatus status = MStatus::MStatusCode::kFailure;
  MFnPlugin k_plugin{obj, "1.0", "Any"};

  //   //创建菜单项
  //   MString pythonResult{};
  //   MGlobal::executePythonCommand(R"(
  // import maya.cmds
  // def doodleCreateMenu():
  //     for me in maya.cmds.lsUI(menus=True,long=True):
  //     	  if maya.cmds.menu(me,q=True,label=True) == "doodle":
  //     		    maya.cmds.menu(me,e=True,deleteAllItems=True)
  //     		    maya.cmds.deleteUI(me)

  //     menu = maya.cmds.menu(parent="MayaWindow",label = "doodle",tearOff=True)
  //     maya.cmds.showWindow()
  //     return menu
  // )");
  //   MGlobal::executePythonCommand("doodleCreateMenu()", pythonResult);

  //添加菜单项
  k_plugin.addMenuItem("Doodle_Motion", "MayaWindow|mainWindowMenu", "doodleCreate", "", false, nullptr, &status);
  CHECK_MSTATUS_AND_RETURN_IT(status);
  //注册命令
  status = k_plugin.registerCommand("doodleCreate", doodle::MayaPlug::doodleCreate::create);
  CHECK_MSTATUS_AND_RETURN_IT(status);
  return status;
}

MStatus uninitializePlugin(MObject obj) {
  MStatus status = MStatus::MStatusCode::kFailure;
  MFnPlugin k_plugin{obj};
  //这里要关闭窗口
  doodle::MayaPlug::doodleCreate::clear_();

  MStringArray menuItems{};
  menuItems.append("Doodle_Motion");
  status = k_plugin.removeMenuItem(menuItems);
  CHECK_MSTATUS_AND_RETURN_IT(status);

  //这一部分是删除菜单项的
  //   MString pythonResult{};
  //   MGlobal::executePythonCommand(R"(import maya.cmds
  // def doodleDeleteMenu():
  //     for me in maya.cmds.lsUI(menus=True,long=True):
  //     	  if maya.cmds.menu(me,q=True,label=True) == "doodle":
  //     		    maya.cmds.menu(me,e=True,deleteAllItems=True)
  //     		    maya.cmds.deleteUI(me)
  // )");

  //   MGlobal::executePythonCommand("doodleDeleteMenu()");
  //   CHECK_MSTATUS_AND_RETURN_IT(status);

  status = k_plugin.deregisterCommand("doodleCreate");
  CHECK_MSTATUS_AND_RETURN_IT(status);
  return status;
}
