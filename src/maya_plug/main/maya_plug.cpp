#include <doodle_lib/doodle_lib_all.h>
#include <doodle_core/core/core_set.h>
#include <doodle_core/core/doodle_lib.h>

#include <doodle_core/lib_warp/std_warp.h>

#include <maya/MDrawRegistry.h>
#include <maya/MFnPlugin.h>
#include <maya/MSceneMessage.h>
#include <maya/MTimerMessage.h>

#include <maya_plug/maya_comm/cam_comm.h>
#include <maya_plug/maya_comm/open_doodle_main.h>
#include <maya_plug/maya_comm/play_blash_comm.h>
#include <maya_plug/maya_comm/reference_comm.h>
#include <maya_plug/maya_comm/file_comm.h>
#include <maya_plug/maya_comm/clear_scene_comm.h>
#include <maya_plug/data/create_hud_node.h>
#include <maya_plug/maya_comm/afterimage_comm.h>
#include <maya_plug/maya_comm/find_duplicate_poly_comm.h>
#include <maya_plug/maya_comm/replace_rig_file_command.h>
#include <maya_plug/maya_comm/upload_files_command.h>
#include <maya_plug/maya_comm/dem_bones_comm.h>
#include <maya_plug/maya_comm/dem_bones_add_weight.h>
#include <maya_plug/maya_comm/create_qcloth_assets.h>
#include <maya_plug/maya_comm/sequence_to_blend_shape_comm.h>
#include <maya_plug/maya_comm/sequence_to_blend_shape_ref_comm.h>

#include <maya_plug/gui/maya_plug_app.h>
#include <maya_plug/maya_render/hud_render_node.h>
#include <maya_plug/maya_render/hud_render_override.h>
#include <maya_plug/logger/maya_logger_info.h>

#include <doodle_app/gui/main_proc_handle.h>
#include <maya_plug/data/maya_register_main.h>

#include <QtCore/QtCore>
#include <QtWidgets/QApplication>
#include <QtWidgets/QWidget>
#include <maya/MQtUtil.h>

#include <stack>
namespace {
const constexpr std::string_view doodle_windows{"doodle_windows"};
const constexpr std::string_view doodle_win_path{"MayaWindow|mainWindowMenu"};
const constexpr std::string_view doolde_hud_render_node{"doolde_hud_render_node"};

MCallbackId clear_callback_id{0};
MCallbackId app_run_id{0};
std::stack<MCallbackId> maya_call_back_id{};

using namespace doodle;
std::shared_ptr<app_command_base> p_doodle_app = nullptr;
std::shared_ptr<::doodle::maya_plug::maya_register> maya_reg{nullptr};

}  // namespace

namespace doodle::maya_plug {
void open_windows() {
  HWND win_id       = reinterpret_cast<HWND>(MQtUtil::mainWindow()->winId());
  auto l_doodle_app = std::make_shared<doodle::maya_plug::maya_plug_app>(doodle::doodle_main_app::in_gui_arg{
      doodle::app_base::in_app_args{::MhInstPlugin, nullptr}, SW_HIDE, win_id});
  p_doodle_app      = l_doodle_app;
}
}  // namespace doodle::maya_plug

MStatus initializePlugin(MObject obj) {
  /**
   * @brief 添加插件注册方法
   */
  MStatus status = MStatus::MStatusCode::kFailure;
  MFnPlugin k_plugin{
      obj,
      "doodle",
      version::build_info::get().version_str.c_str(),
      fmt::format("{}", MAYA_API_VERSION).c_str()};

  auto k_st = MGlobal::mayaState(&status);
  CHECK_MSTATUS_AND_RETURN_IT(status);
  maya_reg = std::make_shared<::doodle::maya_plug::maya_register>();

  switch (k_st) {
    case MGlobal::MMayaState::kBaseUIMode:
    case MGlobal::MMayaState::kInteractive: {
      doodle::maya_plug::open_windows();
      // 注册命令
      status = maya_reg->register_command<::doodle::maya_plug::open_doodle_main>(k_plugin);
      CHECK_MSTATUS(status);

      // 添加菜单项
      k_plugin.addMenuItem(doodle_windows.data(), doodle_win_path.data(), ::doodle::maya_plug::doodleCreate_name, "", false, nullptr, &status);
      CHECK_MSTATUS_AND_RETURN_IT(status);

      /// \brief  自定义hud回调
      maya_reg->register_callback(MSceneMessage::addCallback(
          MSceneMessage::Message::kAfterOpen,
          [](void* clientData) {
            ::doodle::maya_plug::create_hud_node k_c{};
            k_c();
          },
          nullptr,
          &status
      ));
      CHECK_MSTATUS(status);
      maya_reg->register_callback(MSceneMessage::addCallback(
          MSceneMessage::Message::kAfterNew,
          [](void* clientData) {
            ::doodle::maya_plug::create_hud_node k_c{};
            k_c();
          },
          nullptr,
          &status
      ));
      CHECK_MSTATUS(status);
      if (doodle::core_set::get_set().maya_replace_save_dialog) {
        maya_reg->register_callback(
            MSceneMessage::addCheckCallback(
                MSceneMessage::Message::kBeforeSaveCheck,
                [](bool* retCode, void* clientData) {
                  *retCode = maya_plug::clear_scene_comm::show_save_mag();
                },
                nullptr,
                &status
            )
        );
        CHECK_MSTATUS(status);
      }
    } break;
    case MGlobal::MMayaState::kBatch:
    case MGlobal::MMayaState::kLibraryApp:
    default: {
      p_doodle_app = std::make_shared<doodle::app_command_base>(doodle::app_base::in_app_args{::MhInstPlugin, nullptr});
    } break;
  }
  maya_reg->register_callback(MSceneMessage::addCallback(
      MSceneMessage::Message::kMayaExiting,
      [](void* in) {
        if (MGlobal::mayaState() == MGlobal::kInteractive)
          doodle_main_app::Get().stop();
        p_doodle_app.reset();
      },
      nullptr,
      &status
  ));
  CHECK_MSTATUS(status);

  doodle::logger_ctrl::get_log().set_log_name("doodle_maya_plug.txt");
  doodle::logger_ctrl::get_log().add_log_sink(std::make_shared<::doodle::maya_plug::maya_msg_mt>());

  maya_reg->register_callback(MTimerMessage::addTimerCallback(
      0.001,
      [](float elapsedTime, float lastTime, void* clientData) {
        if (p_doodle_app) {
          p_doodle_app->poll_one();
        }
      },
      nullptr, &status
  ));
  CHECK_MSTATUS(status);

  /// 注册自定义hud节点
  status = maya_reg->register_node<doodle::maya_plug::doodle_info_node>(k_plugin);
  CHECK_MSTATUS(status);

  /// 注册自定义渲染覆盖显示hud
  status = maya_reg->register_draw_overrider<
      doodle::maya_plug::doodle_info_node,
      doodle::maya_plug::doodle_info_node_draw_override>();
  CHECK_MSTATUS(status);

  /// 注册拍屏命令
  status = maya_reg->register_command<::doodle::maya_plug::comm_play_blast_maya>(k_plugin);
  CHECK_MSTATUS(status);
  /// 注册创建hud命令
  status = maya_reg->register_command<::doodle::maya_plug::create_hud_node_maya>(k_plugin);
  CHECK_MSTATUS(status);

  /// 注册一些引用命令
  status = maya_reg->register_command<::doodle::maya_plug::create_ref_file_command>(k_plugin);
  CHECK_MSTATUS(status);
  status = maya_reg->register_command<::doodle::maya_plug::ref_file_load_command>(k_plugin);
  CHECK_MSTATUS(status);
  status = maya_reg->register_command<::doodle::maya_plug::ref_file_sim_command>(k_plugin);
  CHECK_MSTATUS(status);
  status = maya_reg->register_command<::doodle::maya_plug::ref_file_export_command>(k_plugin);
  CHECK_MSTATUS(status);
  status = maya_reg->register_command<::doodle::maya_plug::load_project>(k_plugin);
  CHECK_MSTATUS(status);

  /// 导出相机命令注册
  status = maya_reg->register_command<::doodle::maya_plug::export_camera_command>(k_plugin);
  CHECK_MSTATUS(status);
  /// 保存文件命令
  status = maya_reg->register_command<::doodle::maya_plug::comm_file_save>(k_plugin);
  CHECK_MSTATUS(status);
  /// 添加残像命令
  status = maya_reg->register_command<::doodle::maya_plug::afterimage_comm>(k_plugin);
  CHECK_MSTATUS(status);
  /// 添加清除maya场景命令
  status = maya_reg->register_command<::doodle::maya_plug::clear_scene_comm>(k_plugin);
  CHECK_MSTATUS(status);

  /// 添加寻找重复模型命令
  status = maya_reg->register_command<::doodle::maya_plug::find_duplicate_poly_comm>(k_plugin);
  CHECK_MSTATUS(status);
  /// 添加设置缓存命令
  status = maya_reg->register_command<::doodle::maya_plug::set_cloth_cache_path>(k_plugin);
  CHECK_MSTATUS(status);
  /// 添加替换文件命令
  status = maya_reg->register_command<::doodle::maya_plug::replace_rig_file_command>(k_plugin);
  CHECK_MSTATUS(status);
  /// 添加上传文件命令
  status = maya_reg->register_command<::doodle::maya_plug::upload_files_command>(k_plugin);
  CHECK_MSTATUS(status);
  /// 添加解算骨骼命令
  status = maya_reg->register_command<::doodle::maya_plug::dem_bones_comm>(k_plugin);
  CHECK_MSTATUS(status);
  /// 添加解算骨骼命令
  status = maya_reg->register_command<::doodle::maya_plug::dem_bones_add_weight>(k_plugin);
  CHECK_MSTATUS(status);
  /// 添加变形骨骼命令
  status = maya_reg->register_command<::doodle::maya_plug::sequence_to_blend_shape_comm>(k_plugin);
  CHECK_MSTATUS(status);
  /// 添加变形骨骼命令2
  status = maya_reg->register_command<::doodle::maya_plug::sequence_to_blend_shape_ref_comm>(k_plugin);
  CHECK_MSTATUS(status);

  /// 添加创建布料命令
  status = maya_reg->register_command<::doodle::maya_plug::create_qcloth_assets>(k_plugin);
  CHECK_MSTATUS(status);

  /// 等所有命令完成后加载工具架
  switch (k_st) {
    case MGlobal::MMayaState::kInteractive:
      status = MGlobal::executePythonCommandOnIdle(R"(import scripts.Doodle_shelf
scripts.Doodle_shelf.DoodleUIManage.deleteSelf()
scripts.Doodle_shelf.DoodleUIManage.creation()
)");
      CHECK_MSTATUS(status);
      break;
    default:
      break;
  }
  if (!::doodle::core_set::get_set().maya_force_resolve_link) {
    status = MGlobal::executeCommandOnIdle(R"(optionVar -iv FileDialogStyle 1;)");
    CHECK_MSTATUS(status);
  }
  return status;
}

MStatus uninitializePlugin(MObject obj) {
  MStatus status = MStatus::MStatusCode::kFailure;

  MFnPlugin k_plugin{obj};

  auto k_st = MGlobal::mayaState(&status);
  CHECK_MSTATUS_AND_RETURN_IT(status);
  if (!::doodle::core_set::get_set().maya_force_resolve_link) {
    /// \brief
    status = MGlobal::executeCommandOnIdle(R"(optionVar -iv FileDialogStyle 2;)");
    CHECK_MSTATUS(status);
  }
  // 这里要停止app
  p_doodle_app->stop();
  /// 先删除工具架
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
  status = maya_reg->unregister(k_plugin);
  CHECK_MSTATUS(status);

  switch (k_st) {
    case MGlobal::MMayaState::kBaseUIMode:
    case MGlobal::MMayaState::kInteractive: {
      // 这一部分是删除菜单项的
      MStringArray menuItems{};
      menuItems.append(doodle_windows.data());
      status = k_plugin.removeMenuItem(menuItems);
      CHECK_MSTATUS(status);
      break;
    }

    case MGlobal::MMayaState::kBatch:
    case MGlobal::MMayaState::kLibraryApp:
    default:
      break;
  }
  p_doodle_app.reset();
  maya_reg.reset();
  return status;
}
