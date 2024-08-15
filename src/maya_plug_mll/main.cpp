#include <doodle_core/core/core_set.h>
#include <doodle_core/core/doodle_lib.h>
#include <doodle_core/core/program_info.h>
#include <doodle_core/database_task/sqlite_client.h>
#include <doodle_core/platform/win/register_file_type.h>

#include <doodle_app/app/program_options.h>

#include <maya_plug/data/maya_register_main.h>
#include <maya_plug/gui/maya_plug_app.h>
#include <maya_plug/logger/maya_logger_info.h>
#include <maya_plug/maya_comm/add_entt.h>
#include <maya_plug/maya_comm/afterimage_comm.h>
#include <maya_plug/maya_comm/chick_export_fbx.h>
#include <maya_plug/maya_comm/clear_scene_comm.h>
#include <maya_plug/maya_comm/create_qcloth_assets.h>
#include <maya_plug/maya_comm/dem_bones_add_weight.h>
#include <maya_plug/maya_comm/dem_bones_comm.h>
#include <maya_plug/maya_comm/doodle_to_ue_fbx.h>
#include <maya_plug/maya_comm/export_abc_file.h>
#include <maya_plug/maya_comm/file_info_edit.h>
#include <maya_plug/maya_comm/find_duplicate_poly_comm.h>
#include <maya_plug/maya_comm/open_doodle_main.h>
#include <maya_plug/maya_comm/ref_file_export.h>
#include <maya_plug/maya_comm/reference_comm.h>
#include <maya_plug/node/files_info.h>

#include <maya/MFnPlugin.h>
#include <maya/MQtUtil.h>
#include <maya/MSceneMessage.h>
#include <maya/MTimerMessage.h>
#include <stack>
#include <wil/result.h>

namespace {
const constexpr std::string_view doodle_windows{"doodle_windows"};
const constexpr std::string_view doodle_win_path{"MayaWindow|mainWindowMenu"};

using namespace doodle;
std::shared_ptr<app_base> p_doodle_app = nullptr;
std::shared_ptr<::doodle::maya_plug::maya_register> maya_reg{nullptr};

}  // namespace
namespace doodle::maya_plug {

struct find_windows_call {
  HWND maya_wnd{nullptr};
};

HWND find_windows() {
  find_windows_call l_data{};
  // THROW_IF_WIN32_BOOL_FALSE();
  return MQtUtil::nativeWindow(MQtUtil::mainWindow());
  //  ::EnumWindows(
  //      [](HWND hwnd, LPARAM lParam) -> BOOL {
  //        if (::GetCurrentThreadId() == ::GetWindowThreadProcessId(hwnd, nullptr)) {
  //          auto* l_data     = reinterpret_cast<find_windows_call*>(lParam);
  //          l_data->maya_wnd = hwnd;
  //          return FALSE;
  //        } else {
  //          return TRUE;
  //        }
  //      },
  //      reinterpret_cast<LPARAM>(&l_data)
  //  );
  //  return l_data.maya_wnd;
}

class maya_gui_launcher_t {
 public:
  maya_gui_launcher_t()  = default;
  ~maya_gui_launcher_t() = default;

  bool operator()(const argh::parser& in_arh, std::vector<std::shared_ptr<void>>& in_vector) {
    default_logger_raw()->log(log_loc(), level::warn, "开始初始化基本配置");
    core_set_init k_init{};
    default_logger_raw()->log(log_loc(), level::warn, "寻找用户配置文件目录");
    k_init.config_to_user();
    default_logger_raw()->log(log_loc(), level::warn, "读取配置文件");
    k_init.read_file();
    default_logger_raw()->log(log_loc(), level::warn, "寻找到自身exe {}", register_file_type::program_location());

    switch (MGlobal::mayaState()) {
      case MGlobal::MMayaState::kBaseUIMode:
      case MGlobal::MMayaState::kInteractive: {
        g_ctx().get<program_info>().handle_attr(::MhInstPlugin);
        auto l_gui_facet = std::make_shared<maya_facet>();
        in_vector.emplace_back(l_gui_facet);
        HWND win_id = find_windows();
        g_ctx().get<program_info>().parent_windows_attr(win_id);
        l_gui_facet->post();

        /// 在这里我们加载项目
        g_ctx().get<doodle::database_n::file_translator_ptr>()->set_only_ctx(true);

        break;
      }
      case MGlobal::MMayaState::kBatch:
      case MGlobal::MMayaState::kLibraryApp:
      default:
        break;
    }
    return false;
  }
};

}  // namespace doodle::maya_plug

MStatus initializePlugin(MObject obj) {
  /**
   * @brief 添加插件注册方法
   */
  MStatus status = MStatus::MStatusCode::kFailure;
  MFnPlugin k_plugin{
      obj, "doodle", version::build_info::get().version_str.c_str(), fmt::format("{}", MAYA_API_VERSION).c_str()
  };

  maya_reg           = std::make_shared<::doodle::maya_plug::maya_register>();

  using maya_gui_app = doodle::app_plug<doodle::maya_plug::maya_gui_launcher_t>;
  p_doodle_app       = std::make_shared<maya_gui_app>();
  // 注册命令
  status             = maya_reg->register_command<::doodle::maya_plug::open_doodle_main>(k_plugin);
  CHECK_MSTATUS(status);

  // 添加菜单项
  k_plugin.addMenuItem(
      doodle_windows.data(), doodle_win_path.data(), ::doodle::maya_plug::doodleCreate_name, "", false, nullptr, &status
  );
  if (status)
    maya_reg->register_unregister_fun([](MFnPlugin& in_plug) {
      // 这一部分是删除菜单项的
      MStatus status{};
      MStringArray menuItems{};
      menuItems.append(doodle_windows.data());
      status = in_plug.removeMenuItem(menuItems);
      CHECK_MSTATUS(status);
      return status;
    });
  else
    DOODLE_LOG_ERROR(status);

  maya_reg->register_callback(MSceneMessage::addCallback(
      MSceneMessage::Message::kMayaExiting,
      [](void* in) {
        if (auto l_doodle_app = static_cast<app_base*>(in)) {
          l_doodle_app->stop_app();
        }
      },
      p_doodle_app.get(), &status
  ));
  CHECK_MSTATUS(status);

  doodle::g_logger_ctrl().add_log_sink(std::make_shared<::doodle::maya_plug::maya_msg_mt>(), "maya_plug");

  maya_reg->register_callback(MTimerMessage::addTimerCallback(
      0.001,
      [](float elapsedTime, float lastTime, void* clientData) {
        if (auto l_doodle_app = static_cast<app_base*>(clientData)) {
          l_doodle_app->poll_one();
        }
      },
      p_doodle_app.get(), &status
  ));
  CHECK_MSTATUS(status);

  CHECK_MSTATUS(status);
  status = maya_reg->register_node<doodle::maya_plug::doodle_file_info>(k_plugin);
  CHECK_MSTATUS(status);
  /// 添加文件编辑命令
  status = maya_reg->register_command<::doodle::maya_plug::file_info_edit>(k_plugin);
  CHECK_MSTATUS(status);

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
  /// 添加导出命令
  status = maya_reg->register_command<::doodle::maya_plug::ref_file_export>(k_plugin);
  CHECK_MSTATUS(status);

  /// 添加解算骨骼命令
  status = maya_reg->register_command<::doodle::maya_plug::dem_bones_comm>(k_plugin);
  CHECK_MSTATUS(status);
  /// 添加解算骨骼命令
  status = maya_reg->register_command<::doodle::maya_plug::dem_bones_add_weight>(k_plugin);
  CHECK_MSTATUS(status);

  /// 添加创建布料命令
  status = maya_reg->register_command<::doodle::maya_plug::create_qcloth_assets>(k_plugin);
  CHECK_MSTATUS(status);

  ///  添加自定义fbx导出
  status = maya_reg->register_command<::doodle::maya_plug::doodle_to_ue_fbx>(k_plugin);
  CHECK_MSTATUS(status);
  ///  添加检查fbx导出命令
  status = maya_reg->register_command<::doodle::maya_plug::chick_export_fbx>(k_plugin);
  CHECK_MSTATUS(status);
  /// 添加abc导出命令
  status = maya_reg->register_command<::doodle::maya_plug::export_abc_file>(k_plugin);
  CHECK_MSTATUS(status);
  /// 等所有命令完成后加载工具架
  status = MGlobal::executePythonCommandOnIdle(R"(import scripts.Doodle_shelf
scripts.Doodle_shelf.DoodleUIManage.deleteSelf()
scripts.Doodle_shelf.DoodleUIManage.creation()
)");
  CHECK_MSTATUS(status);

  maya_reg->register_unregister_fun([](MFnPlugin& in_plug) {
    return MGlobal::executePythonCommand(R"(import scripts.Doodle_shelf
scripts.Doodle_shelf.DoodleUIManage.deleteSelf()
)");
  });

  if (!::doodle::core_set::get_set().maya_force_resolve_link) {
    status = MGlobal::executeCommandOnIdle(R"(optionVar -iv FileDialogStyle 1;)");
    CHECK_MSTATUS(status);
    maya_reg->register_unregister_fun([](MFnPlugin& in_plug) {
      return MGlobal::executeCommandOnIdle(R"(optionVar -iv FileDialogStyle 2;)");
    });
  }
  return status;
}

MStatus uninitializePlugin(MObject obj) {
  MStatus status{};

  MFnPlugin k_plugin{obj};
  // 这里要停止app
  if (p_doodle_app) {
    p_doodle_app->stop_app();
    p_doodle_app->run();
  }
  status = maya_reg->unregister(k_plugin);
  CHECK_MSTATUS(status);
  p_doodle_app.reset();
  maya_reg.reset();
  return status;
}
