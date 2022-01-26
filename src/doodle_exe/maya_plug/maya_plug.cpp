#include <doodle_lib/core/core_set.h>
#include <doodle_lib/core/doodle_lib.h>
#include <doodle_lib/doodle_app.h>
#include <doodle_lib/lib_warp/std_warp.h>

#include <maya/MDrawRegistry.h>
#include <maya/MFnPlugin.h>
#include <maya/MSceneMessage.h>
#include <maya/MTimerMessage.h>

#include <maya_plug/maya_comm/cam_comm.h>
#include <maya_plug/maya_comm/open_doodle_main.h>
#include <maya_plug/maya_comm/play_blash_comm.h>
#include <maya_plug/maya_comm/reference_comm.h>
#include <maya_plug/maya_comm/file_comm.h>
#include <maya_plug/data/create_hud_node.h>
#include <maya_plug/maya_comm/afterimage_comm.h>

#include <maya_plug/gui/maya_plug_app.h>
#include <maya_plug/maya_render/hud_render_node.h>
#include <maya_plug/maya_render/hud_render_override.h>
#include <maya_plug/logger/maya_logger_info.h>
namespace {
const constexpr std::string_view doodle_windows{"doodle_windows"};
const constexpr std::string_view doodle_win_path{"MayaWindow|mainWindowMenu"};
const constexpr std::string_view doolde_hud_render_node{"doolde_hud_render_node"};

MCallbackId clear_callback_id{0};
MCallbackId app_run_id{0};
MCallbackId create_hud_id{0};
MCallbackId create_hud_id_2{0};

using namespace doodle;
std::shared_ptr<app_base> p_doodle_app = nullptr;

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

  switch (k_st) {
    case MGlobal::MMayaState::kBaseUIMode:
    case MGlobal::MMayaState::kInteractive: {
      p_doodle_app = std::make_shared<doodle::maya_plug::maya_plug_app>(::MhInstPlugin);
      app::Get().hide_windows();

      //注册命令
      status = ::doodle::maya_plug::open_doodle_main::registerCommand(k_plugin);
      CHECK_MSTATUS_AND_RETURN_IT(status);

      //添加菜单项
      k_plugin.addMenuItem(doodle_windows.data(),
                           doodle_win_path.data(),
                           ::doodle::maya_plug::doodleCreate_name,
                           "",
                           false,
                           nullptr,
                           &status);
      CHECK_MSTATUS_AND_RETURN_IT(status);

      /// \brief  自定义hud回调
      create_hud_id = MSceneMessage::addCallback(
          MSceneMessage::Message::kAfterOpen,
          [](void* clientData) {
            ::doodle::maya_plug::create_hud_node k_c{};
            k_c();
          },
          nullptr,
          &status);
      CHECK_MSTATUS_AND_RETURN_IT(status);
      create_hud_id_2 = MSceneMessage::addCallback(
          MSceneMessage::Message::kAfterNew,
          [](void* clientData) {
            ::doodle::maya_plug::create_hud_node k_c{};
            k_c();
          },
          nullptr,
          &status);
      CHECK_MSTATUS_AND_RETURN_IT(status);
    } break;
    case MGlobal::MMayaState::kBatch:
    case MGlobal::MMayaState::kLibraryApp:
    default:
      p_doodle_app = std::make_shared<doodle::app_command_base>();
      break;
  }
  clear_callback_id = MSceneMessage::addCallback(
      MSceneMessage::Message::kMayaExiting,
      [](void* in) {
        app::Get().stop();
        p_doodle_app.reset();
      },
      nullptr,
      &status);
  CHECK_MSTATUS_AND_RETURN_IT(status);
  doodle::logger_ctrl::get_log().set_log_name("doodle_maya_plug.txt");
  doodle::logger_ctrl::get_log().add_log_sink(new_object<::doodle::maya_plug::maya_msg_mt>());

  app_run_id = MTimerMessage::addTimerCallback(
      0.001,
      [](float elapsedTime, float lastTime, void* clientData) {
        if (p_doodle_app->stop_) {
          return;
        }
        p_doodle_app->loop_one();
      },
      nullptr, &status);

  CHECK_MSTATUS_AND_RETURN_IT(status);

  /// 注册自定义hud节点
  status = k_plugin.registerNode(
      doolde_hud_render_node.data(),
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
  status = maya_plug::comm_play_blast_maya::registerCommand(k_plugin);
  CHECK_MSTATUS_AND_RETURN_IT(status);
  /// 注册创建hud命令
  status = ::doodle::maya_plug::create_hud_node_maya::registerCommand(k_plugin);
  CHECK_MSTATUS_AND_RETURN_IT(status);

  /// 注册一些引用命令
  status = ::doodle::maya_plug::create_ref_file_command::registerCommand(k_plugin);
  CHECK_MSTATUS_AND_RETURN_IT(status);
  status = ::doodle::maya_plug::ref_file_load_command::registerCommand(k_plugin);
  CHECK_MSTATUS_AND_RETURN_IT(status);
  status = ::doodle::maya_plug::ref_file_sim_command::registerCommand(k_plugin);
  CHECK_MSTATUS_AND_RETURN_IT(status);
  status = ::doodle::maya_plug::ref_file_export_command::registerCommand(k_plugin);
  CHECK_MSTATUS_AND_RETURN_IT(status);
  status = ::doodle::maya_plug::load_project::registerCommand(k_plugin);
  CHECK_MSTATUS_AND_RETURN_IT(status);

  /// 导出相机命令注册
  status = ::doodle::maya_plug::export_camera_command::registerCommand(k_plugin);
  CHECK_MSTATUS_AND_RETURN_IT(status);
  /// 保存文件命令
  status = ::doodle::maya_plug::comm_file_save::registerCommand(k_plugin);
  CHECK_MSTATUS_AND_RETURN_IT(status);
  /// 添加残像命令
  status = ::doodle::maya_plug::afterimage_comm::registerCommand(k_plugin);
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

  /// 取消残像命令
  status = ::doodle::maya_plug::afterimage_comm::deregisterCommand(k_plugin);
  CHECK_MSTATUS_AND_RETURN_IT(status);

  /// 保存文件命令取消注册
  status = ::doodle::maya_plug::comm_file_save::deregisterCommand(k_plugin);
  CHECK_MSTATUS_AND_RETURN_IT(status);

  /// 导出相机命令取消注册
  status = ::doodle::maya_plug::export_camera_command::deregisterCommand(k_plugin);
  CHECK_MSTATUS_AND_RETURN_IT(status);

  /// 去除解算命令
  status = ::doodle::maya_plug::load_project::deregisterCommand(k_plugin);
  CHECK_MSTATUS_AND_RETURN_IT(status);
  status = ::doodle::maya_plug::create_ref_file_command::deregisterCommand(k_plugin);
  CHECK_MSTATUS_AND_RETURN_IT(status);
  status = ::doodle::maya_plug::ref_file_load_command::deregisterCommand(k_plugin);
  CHECK_MSTATUS_AND_RETURN_IT(status);
  status = ::doodle::maya_plug::ref_file_sim_command::deregisterCommand(k_plugin);
  CHECK_MSTATUS_AND_RETURN_IT(status);
  status = ::doodle::maya_plug::ref_file_export_command::deregisterCommand(k_plugin);
  CHECK_MSTATUS_AND_RETURN_IT(status);

  /// 去掉hud命令
  status = ::doodle::maya_plug::create_hud_node_maya::deregisterCommand(k_plugin);
  CHECK_MSTATUS_AND_RETURN_IT(status);
  /// 去掉拍屏命令
  status = maya_plug::comm_play_blast_maya::deregisterCommand(k_plugin);
  CHECK_MSTATUS_AND_RETURN_IT(status);
  /// 去掉渲染覆盖命令
  status = MDrawRegistry::deregisterGeometryOverrideCreator(
      doodle::maya_plug::doodle_info_node::drawDbClassification,
      doodle::maya_plug::doodle_info_node::drawRegistrantId);
  CHECK_MSTATUS_AND_RETURN_IT(status);
  /// 去掉hud节点
  status = k_plugin.deregisterNode(doodle::maya_plug::doodle_info_node::doodle_id);
  CHECK_MSTATUS_AND_RETURN_IT(status);

  /// 去除运行回调
  status = MMessage::removeCallback(app_run_id);
  CHECK_MSTATUS_AND_RETURN_IT(status);

  //删除清除回调回调
  status = MMessage::removeCallback(clear_callback_id);
  CHECK_MSTATUS_AND_RETURN_IT(status);

  switch (k_st) {
    case MGlobal::MMayaState::kBaseUIMode:
    case MGlobal::MMayaState::kInteractive: {
      /// 去除hud回调
      status = MMessage::removeCallback(create_hud_id);
      CHECK_MSTATUS_AND_RETURN_IT(status);
      status = MMessage::removeCallback(create_hud_id_2);
      CHECK_MSTATUS_AND_RETURN_IT(status);

      //这一部分是删除菜单项的
      MStringArray menuItems{};
      menuItems.append(doodle_windows.data());
      status = k_plugin.removeMenuItem(menuItems);
      CHECK_MSTATUS_AND_RETURN_IT(status);

      ///去除命令
      status = ::doodle::maya_plug::open_doodle_main::deregisterCommand(k_plugin);
      CHECK_MSTATUS_AND_RETURN_IT(status);
      break;
    }

    case MGlobal::MMayaState::kBatch:
    case MGlobal::MMayaState::kLibraryApp:
    default:
      break;
  }
  // 卸载命令

  //这里要关闭窗口或者清理库
  p_doodle_app->stop();
  p_doodle_app.reset();

  return status;
}
