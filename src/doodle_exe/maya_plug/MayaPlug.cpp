#include <maya/MFnPlugin.h>
#include <maya/MSceneMessage.h>
#include <maya_plug/MotionMayaPlugInit.h>

namespace {
const static std::string doodle_windows{"doodle_windows"};
const static std::string doodle_win_path{"MayaWindow|mainWindowMenu"};
const static std::string doodle_create{"doodleCreate"};
static MCallbackId clear_callback_id{0};

}  // namespace

MStatus initializePlugin(MObject obj) {
  /**
   * @brief 添加插件注册方法
   */
  MStatus status = MStatus::MStatusCode::kFailure;
  MFnPlugin k_plugin{obj, "doodle",
                     fmt::format("{}.{}.{}.{}",
                                 Doodle_VERSION_MAJOR,
                                 Doodle_VERSION_MINOR,
                                 Doodle_VERSION_PATCH,
                                 Doodle_VERSION_TWEAK)
                         .c_str(),
                     fmt::format("{}", MAYA_API_VERSION).c_str()};

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

  //注册命令
  status = k_plugin.registerCommand(doodle_create.c_str(), doodle::MayaPlug::doodleCreate::create);
  CHECK_MSTATUS_AND_RETURN_IT(status);

  //添加菜单项
  k_plugin.addMenuItem(doodle_windows.c_str(),
                       doodle_win_path.c_str(),
                       doodle_create.c_str(),
                       "",
                       false,
                       nullptr,
                       &status);
  CHECK_MSTATUS_AND_RETURN_IT(status);

  clear_callback_id = MSceneMessage::addCallback(
      MSceneMessage::Message::kMayaExiting,
      [](void* in) {
        ::doodle::MayaPlug::doodleCreate::clear_();
      },
      nullptr,
      &status);
  CHECK_MSTATUS_AND_RETURN_IT(status);
  return status;
}

MStatus uninitializePlugin(MObject obj) {
  MStatus status = MStatus::MStatusCode::kFailure;

  MFnPlugin k_plugin{obj};
  //删除回调
  status = MMessage::removeCallback(clear_callback_id);
  CHECK_MSTATUS_AND_RETURN_IT(status);

  //这里要关闭窗口
  doodle::MayaPlug::doodleCreate::clear_();

  //这一部分是删除菜单项的
  MStringArray menuItems{};
  menuItems.append(doodle_windows.c_str());
  status = k_plugin.removeMenuItem(menuItems);
  CHECK_MSTATUS_AND_RETURN_IT(status);

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

  // 卸载命令
  status = k_plugin.deregisterCommand(doodle_create.c_str());
  CHECK_MSTATUS_AND_RETURN_IT(status);
  return status;
}
