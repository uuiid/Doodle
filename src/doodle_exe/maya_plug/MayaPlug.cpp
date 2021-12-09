#include <doodle_lib/core/core_set.h>
#include <doodle_lib/core/doodle_lib.h>
#include <doodle_lib/doodle_app.h>
#include <doodle_lib/lib_warp/std_warp.h>

#include <maya/MDrawRegistry.h>
#include <maya/MFnPlugin.h>
#include <maya/MSceneMessage.h>
#include <maya/MTimerMessage.h>

#include <maya_plug/MotionMayaPlugInit.h>
#include <maya_plug/command/create_hud_node.h>
#include <maya_plug/gui/action/comm_play_blast.h>
#include <maya_plug/gui/maya_plug_app.h>
#include <maya_plug/maya_render/hud_render_node.h>
#include <maya_plug/gui/action/reference_attr_setting.h>
#include <maya_plug/maya_render/hud_render_override.h>
#include <maya_plug/logger/maya_logger_info.h>
namespace {
const static std::string doodle_windows{"doodle_windows"};
const static std::string doodle_win_path{"MayaWindow|mainWindowMenu"};
const static std::string doodle_create{"doodleCreate"};
const static std::string doolde_hud_render_node{"doolde_hud_render_node"};

static MCallbackId clear_callback_id{0};
static MCallbackId app_run_id{0};
static MCallbackId create_hud_id{0};

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

  auto k_st = MGlobal::mayaState(&status);
  CHECK_MSTATUS_AND_RETURN_IT(status);

  p_doodle_lib = doodle::new_object<doodle::doodle_lib>();
  doodle::logger_ctrl::get_log().set_log_name("doodle_maya_plug.txt");
  doodle::logger_ctrl::get_log().add_log_sink(new_object<::doodle::maya_plug::maya_msg_mt>());
  doodle::core_set_init k_init{};
  k_init.find_cache_dir();
  k_init.config_to_user();
  k_init.read_file();
  p_doodle_lib->init_gui();

  clear_callback_id = MSceneMessage::addCallback(
      MSceneMessage::Message::kMayaExiting,
      [](void* in) {
        doodle_maya_clear();
      },
      nullptr,
      &status);
  CHECK_MSTATUS_AND_RETURN_IT(status);
  switch (k_st) {
    case MGlobal::MMayaState::kBaseUIMode: {
    }
    case MGlobal::MMayaState::kInteractive: {
      p_doodle_app         = doodle::new_object<doodle::maya_plug::maya_plug_app>();
      p_doodle_app->p_done = true;
      p_doodle_app->hide_windows();

      //注册命令
      status = k_plugin.registerCommand(doodle_create.c_str(),
                                        doodle::MayaPlug::doodleCreate::create);
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

      create_hud_id = MSceneMessage::addCallback(
          MSceneMessage::Message::kAfterOpen,
          [](void* clientData) {
            ::doodle::maya_plug::create_hud_node k_c{};
            k_c();
          },
          nullptr,
          &status);
      CHECK_MSTATUS_AND_RETURN_IT(status);

      break;
    }

    case MGlobal::MMayaState::kBatch: {
      break;
    }
    case MGlobal::MMayaState::kLibraryApp: {
      break;
    }
    default:
      break;
  }
  /// 注册自定义hud节点
  status = k_plugin.registerNode(
      doolde_hud_render_node.c_str(),
      doodle::maya_plug::doodle_info_node::doodle_id,
      &doodle::maya_plug::doodle_info_node::creator,
      &doodle::maya_plug::doodle_info_node::initialize,
      MPxNode::kLocatorNode,
      &doodle::maya_plug::doodle_info_node::drawDbClassification);
  CHECK_MSTATUS_AND_RETURN_IT(status);
  /// 注册自定义渲染覆盖显示hud
  status = MHWRender::MDrawRegistry::registerDrawOverrideCreator(
      doodle::maya_plug::doodle_info_node::drawDbClassification,
      doodle::maya_plug::doodle_info_node::drawRegistrantId,
      doodle::maya_plug::doodle_info_node_draw_override::Creator);
  CHECK_MSTATUS_AND_RETURN_IT(status);

  /// 注册拍屏命令
  status = k_plugin.registerCommand(
      ::doodle::maya_plug::comm_play_blast_maya::comm_name,
      &::doodle::maya_plug::comm_play_blast_maya::creator,
      &::doodle::maya_plug::comm_play_blast_maya::syntax);
  CHECK_MSTATUS_AND_RETURN_IT(status);
  /// 注册创建hud命令
  status = k_plugin.registerCommand(
      ::doodle::maya_plug::create_hud_node_maya::comm_name,
      &::doodle::maya_plug::create_hud_node_maya::creator);
  CHECK_MSTATUS_AND_RETURN_IT(status);

  ///
  status = k_plugin.registerCommand(
      ::doodle::maya_plug::sim_cloth::comm_name,
      &::doodle::maya_plug::sim_cloth::creator,
      &::doodle::maya_plug::sim_cloth::syntax);
  CHECK_MSTATUS_AND_RETURN_IT(status);

  /// 等所有命令完成后加载工具架
  switch (k_st) {
    case MGlobal::MMayaState::kInteractive:
      status = MGlobal::executePythonCommandOnIdle(R"(import scripts.Doodle_shelf
scripts.Doodle_shelf.DoodleUIManage.creation()
)");
      CHECK_MSTATUS(status);
      break;
    default:
      break;
  }

  return status;
}

MStatus uninitializePlugin(MObject obj) {
  MStatus status = MStatus::MStatusCode::kFailure;

  MFnPlugin k_plugin{obj};

  auto k_st = MGlobal::mayaState(&status);
  CHECK_MSTATUS_AND_RETURN_IT(status);

  ///先删除工具架
  switch (k_st) {
    case MGlobal::MMayaState::kInteractive:
      status = MGlobal::executePythonCommand(R"(import scripts.Doodle_shelf
scripts.Doodle_shelf.DoodleUIManage.deleteSelf()
)");
      CHECK_MSTATUS(status);
      break;
    default:
      break;
  }
  /// 去除解算命令
  status = k_plugin.deregisterCommand(
      ::doodle::maya_plug::sim_cloth::comm_name);
  CHECK_MSTATUS_AND_RETURN_IT(status);

  /// 去掉hud命令
  status = k_plugin.deregisterCommand(
      ::doodle::maya_plug::create_hud_node_maya::comm_name);
  CHECK_MSTATUS_AND_RETURN_IT(status);
  /// 去掉拍屏命令
  status = k_plugin.deregisterCommand(
      ::doodle::maya_plug::comm_play_blast_maya::comm_name);
  CHECK_MSTATUS_AND_RETURN_IT(status);
  /// 去掉渲染覆盖命令
  status = MDrawRegistry::deregisterGeometryOverrideCreator(
      doodle::maya_plug::doodle_info_node::drawDbClassification,
      doodle::maya_plug::doodle_info_node::drawRegistrantId);
  CHECK_MSTATUS_AND_RETURN_IT(status);
  /// 去掉hud节点
  status = k_plugin.deregisterNode(doodle::maya_plug::doodle_info_node::doodle_id);
  CHECK_MSTATUS_AND_RETURN_IT(status);

  switch (k_st) {
    case MGlobal::MMayaState::kBaseUIMode: {
    }
    case MGlobal::MMayaState::kInteractive: {
      /// 去除hud回调
      status = MMessage::removeCallback(create_hud_id);
      CHECK_MSTATUS_AND_RETURN_IT(status);

      /// 去除运行回调
      status = MMessage::removeCallback(app_run_id);
      CHECK_MSTATUS_AND_RETURN_IT(status);

      //这一部分是删除菜单项的
      MStringArray menuItems{};
      menuItems.append(doodle_windows.c_str());
      status = k_plugin.removeMenuItem(menuItems);
      CHECK_MSTATUS_AND_RETURN_IT(status);

      ///去除命令
      status = k_plugin.deregisterCommand(doodle_create.c_str());
      CHECK_MSTATUS_AND_RETURN_IT(status);
      break;
    }

    case MGlobal::MMayaState::kBatch: {
      break;
    }
    case MGlobal::MMayaState::kLibraryApp: {
      break;
    }
    default:
      break;
  }
  // 卸载命令
  //删除清除回调回调
  status = MMessage::removeCallback(clear_callback_id);
  CHECK_MSTATUS_AND_RETURN_IT(status);

  //这里要关闭窗口或者清理库
  doodle_maya_clear();

  return status;
}
