#include "doodle_core/core/app_base.h"
#include <doodle_core/core/core_set.h>
#include <doodle_core/core/doodle_lib.h>
#include <doodle_core/platform/win/register_file_type.h>

#include <maya_plug/data/maya_register_main.h>
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
#include <stack>
#include <wil/result.h>

namespace {

using namespace doodle;
std::shared_ptr<app_base> p_doodle_app = nullptr;
std::shared_ptr<::doodle::maya_plug::maya_register> maya_reg{nullptr};

}  // namespace

MStatus initializePlugin(MObject obj) {
  /**
   * @brief 添加插件注册方法
   */
  MStatus status = MStatus::MStatusCode::kSuccess;
  MFnPlugin k_plugin{
      obj, "doodle", version::build_info::get().version_str.c_str(), fmt::format("{}", MAYA_API_VERSION).c_str()
  };

  maya_reg     = std::make_shared<::doodle::maya_plug::maya_register>();
  p_doodle_app = std::make_shared<app_base>();

  doodle::g_logger_ctrl().add_log_sink(std::make_shared<::doodle::maya_plug::maya_msg_mt>(), "maya_plug");

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

import doodle.main
doodle.main.main.add_menu()
)");
  CHECK_MSTATUS(status);

  maya_reg->register_unregister_fun([](MFnPlugin& in_plug) {
    return MGlobal::executePythonCommand(R"(import scripts.Doodle_shelf
scripts.Doodle_shelf.DoodleUIManage.deleteSelf()

import doodle.main
doodle.main.main.remove_menu()
)");
  });

  status = MGlobal::executeCommandOnIdle(R"(optionVar -iv FileDialogStyle 1;)");
  CHECK_MSTATUS(status);
  maya_reg->register_unregister_fun([](MFnPlugin& in_plug) {
    return MGlobal::executeCommandOnIdle(R"(optionVar -iv FileDialogStyle 2;)");
  });
  return status;
}

MStatus uninitializePlugin(MObject obj) {
  MStatus status{};

  MFnPlugin k_plugin{obj};
  status = maya_reg->unregister(k_plugin);
  CHECK_MSTATUS(status);
  maya_reg.reset();
  return status;
}
