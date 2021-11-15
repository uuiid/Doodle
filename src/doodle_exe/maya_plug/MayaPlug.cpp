#include <doodle_lib/core/core_set.h>
#include <doodle_lib/core/doodle_lib.h>
#include <doodle_lib/doodle_app.h>
#include <doodle_lib/lib_warp/std_warp.h>
#include <maya/MFnPlugin.h>
#include <maya/MSceneMessage.h>
#include <maya/MTimerMessage.h>
#include <maya_plug/MotionMayaPlugInit.h>
#include <maya_plug/gui/maya_plug_app.h>
#include <maya_plug/maya_render/hud_render_override.h>

namespace {
const static std::string doodle_windows{"doodle_windows"};
const static std::string doodle_win_path{"MayaWindow|mainWindowMenu"};
const static std::string doodle_create{"doodleCreate"};
const static std::string doolde_hud_render_override{"doolde_hud_render_override"};
static doodle::hud_render* doodle_hud_render{nullptr};

static MCallbackId clear_callback_id{0};
static MCallbackId app_run_id{0};

using namespace doodle;
static doodle_lib_ptr p_doodle_lib              = nullptr;
static std::shared_ptr<doodle_app> p_doodle_app = nullptr;

void doodle_maya_clear() {
  if (p_doodle_app) {
    p_doodle_app->p_done = true;
    p_doodle_app.reset();
  }
  if (p_doodle_lib)
    p_doodle_lib.reset();
}
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

  auto k_st = MGlobal::mayaState(&status);
  CHECK_MSTATUS_AND_RETURN_IT(status);

  switch (k_st) {
    case MGlobal::MMayaState::kInteractive: {
      p_doodle_lib = doodle::new_object<doodle::doodle_lib>();
      doodle::logger_ctrl::get_log().set_log_name("doodle_maya_plug.txt");
      doodle::core_set_init k_init{};
      k_init.find_cache_dir();
      k_init.config_to_user();
      k_init.read_file();
      p_doodle_lib->init_gui();

      p_doodle_app         = doodle::new_object<doodle::maya_plug::maya_plug_app>();
      p_doodle_app->p_done = true;
      p_doodle_app->hide_windows();

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
            doodle_maya_clear();
          },
          nullptr,
          &status);
      CHECK_MSTATUS_AND_RETURN_IT(status);
      app_run_id = MTimerMessage::addTimerCallback(
          0.001,
          [](float elapsedTime, float lastTime, void* clientData) {
            if (p_doodle_app->p_done) {
              p_doodle_app->hide_windows();
              return;
            }
            p_doodle_app->loop_one();
          },
          nullptr, &status);

      CHECK_MSTATUS_AND_RETURN_IT(status);

      // MHWRender::MRenderer* k_r = MHWRender::MRenderer::theRenderer();
      // if (k_r) {
      //   auto k_o_r = new hud_render_override(doolde_hud_render_override.c_str());
      //   if (k_o_r) {
      //     status = k_r->registerOverride(k_o_r);
      //     CHECK_MSTATUS_AND_RETURN_IT(status);
      //   }
      // }
      MHWRender::MRenderer* k_r = MHWRender::MRenderer::theRenderer();
      if (k_r) {
        MRenderOperationList mOperations;
        k_r->getStandardViewportOperations(mOperations);
        // auto k_stander_hud_index = mOperations.indexOf(MRenderOperation::kStandardHUDName);
        // auto k_stander_hud       = mOperations[k_stander_hud_index];

        // std::cout << k_stander_hud->inputTargets() << std::endl;
        // std::cout << k_stander_hud->outputTargets() << std::endl;
        // auto k_stander_hud_next = mOperations[k_stander_hud_index + 1];
        // std::cout << k_stander_hud_next->inputTargets() << std::endl;
        // std::cout << k_stander_hud_next->outputTargets() << std::endl;

        doodle_hud_render = new hud_render{doolde_hud_render_override.c_str()};

        if (doodle_hud_render)
          mOperations.insertBefore(MRenderOperation::kStandardHUDName, doodle_hud_render);
      }
      break;
    }

    default:
      return status;
  }
  return status;
}

MStatus uninitializePlugin(MObject obj) {
  MStatus status = MStatus::MStatusCode::kFailure;

  MFnPlugin k_plugin{obj};

  auto k_st = MGlobal::mayaState(&status);
  CHECK_MSTATUS_AND_RETURN_IT(status);

  switch (k_st) {
    case MGlobal::MMayaState::kInteractive: {
      //这里要关闭窗口
      doodle_maya_clear();

      //删除回调
      status = MMessage::removeCallback(clear_callback_id);
      CHECK_MSTATUS_AND_RETURN_IT(status);

      status = MMessage::removeCallback(app_run_id);
      CHECK_MSTATUS_AND_RETURN_IT(status);

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

      // MHWRender::MRenderer* k_r = MHWRender::MRenderer::theRenderer();
      // if (k_r) {
      //   auto k_o_r = k_r->findRenderOverride(doolde_hud_render_override.c_str());
      //   if (k_o_r) {
      //     status = k_r->deregisterOverride(k_o_r);
      //     CHECK_MSTATUS_AND_RETURN_IT(status);
      //     delete k_o_r;
      //   }
      // }
      MHWRender::MRenderer* k_r = MHWRender::MRenderer::theRenderer();
      if (k_r) {
        MRenderOperationList mOperations;
        k_r->getStandardViewportOperations(mOperations);
        mOperations.remove(doolde_hud_render_override.c_str());
      }
      break;
    }
    default:
      return status;
  }

  return status;
}
