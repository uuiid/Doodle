#include <MotionMayaPlugInit.h>
#include <maya/MFnPlugin.h>

namespace {
const static std::string doodle_windows{"doodle_windows"};
const static std::string doodle_win_path{"MayaWindow|mainWindowMenu"};
const static std::string doodle_create{"doodleCreate"};


}

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
  k_plugin.addMenuItem(doodle_windows.c_str(),
                       doodle_win_path.c_str(),
                       doodle_create.c_str(),
                       "",
                       false,
                       nullptr,
                       &status);
  CHECK_MSTATUS_AND_RETURN_IT(status);
  //注册命令
  status = k_plugin.registerCommand(doodle_create.c_str(), doodle::MayaPlug::doodleCreate::create);
  CHECK_MSTATUS_AND_RETURN_IT(status);
  return status;
}

MStatus uninitializePlugin(MObject obj) {
  MStatus status = MStatus::MStatusCode::kFailure;
  MFnPlugin k_plugin{obj};
  //这里要关闭窗口
  doodle::MayaPlug::doodleCreate::clear_();

  MStringArray menuItems{};
  menuItems.append(doodle_windows.c_str());
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

  status = k_plugin.deregisterCommand(doodle_create.c_str());
  CHECK_MSTATUS_AND_RETURN_IT(status);
  return status;
}
