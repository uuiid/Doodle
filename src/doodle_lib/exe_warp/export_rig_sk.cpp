#include "export_rig_sk.h"

#include "doodle_core/configure/static_value.h"
#include "doodle_core/core/core_set.h"
#include "doodle_core/core/file_sys.h"
#include "doodle_core/exception/exception.h"
#include "doodle_core/metadata/entity_type.h"

#include <doodle_lib/exe_warp/import_and_render_ue.h>
#include <doodle_lib/exe_warp/maya_exe.h>
#include <doodle_lib/exe_warp/ue_exe.h>

#include <boost/asio/awaitable.hpp>

#include <filesystem>

namespace doodle {

boost::asio::awaitable<void> export_rig_sk_arg::run() {
  if (asset_type_id_.is_nil()) throw_exception(doodle_error{"asset_type_id 不能为空"});

  auto l_arg = std::make_shared<maya_exe_ns::export_rig_arg>();
  l_arg->set_file_path(maya_file_);
  l_arg->set_logger(logger_ptr_);
  co_await l_arg->async_run_maya();
  auto l_maya_file = l_arg->get_out_arg();
  FSys::path l_import_game_path{doodle_config::ue4_game};
  if (asset_type_id_ == asset_type::get_character_id()) {
    l_import_game_path /= "Character";
    l_import_game_path /= pin_yin_ming_cheng_;
    l_import_game_path /= "Meshs";
  } else if (asset_type_id_ == asset_type::get_prop_id()) {
    l_import_game_path /= "Prop";
    l_import_game_path /= pin_yin_ming_cheng_;
    l_import_game_path /= "Mesh";
  } else
    throw_exception(doodle_error{"不支持的类型 {}", asset_type_id_});

  if (l_maya_file.out_file_list.empty()) throw_exception(doodle_error{"文件 {}, 未能输出骨架fbx", maya_file_});
  auto l_fbx = l_maya_file.out_file_list.front().out_file;

  if (!FSys::exists(l_fbx)) throw_exception(doodle_error{"文件 {} 不存在", l_fbx});
  {
    std::string l_new_fbx_name{};
    if (asset_type_id_ == asset_type::get_character_id()) {
      l_new_fbx_name = fmt::format("SK_Ch{}.fbx", bian_hao_, pin_yin_ming_cheng_);
    } else if (asset_type_id_ == asset_type::get_prop_id()) {
      l_new_fbx_name = fmt::format("SK_{}{}{}.fbx", pin_yin_ming_cheng_, ban_ben_.empty() ? "" : "_", ban_ben_);
    }
    logger_ptr_->info("重命名 {} 为 {}", l_fbx.filename(), l_new_fbx_name);
    FSys::rename(l_fbx, l_fbx.parent_path() / l_new_fbx_name);
    l_fbx = l_fbx.parent_path() / l_new_fbx_name;
  }

  nlohmann::json l_json{};
  l_json            = import_and_render_ue_ns::import_skin_file{.fbx_file_ = l_fbx, .import_dir_ = l_import_game_path};
  auto l_tmp_path   = FSys::write_tmp_file("ue_import", l_json.dump(), ".json");

  auto l_ue_project = ue_exe_ns::find_ue_project_file(ue_path_);
  if (l_ue_project.empty()) throw doodle_error{"无法找到UE项目文件 {}", ue_path_};
  auto l_local_ue_project =
      core_set::get_set().get_cache_root(project_name_ / l_ue_project.stem()) / l_ue_project.filename();
  auto l_local_ue_project_dir = l_local_ue_project.parent_path();

  FSys::copy_diff(
      l_ue_project.parent_path() / doodle_config::ue4_content, l_local_ue_project_dir / doodle_config::ue4_content,
      logger_ptr_
  );
  FSys::copy_diff(
      l_ue_project.parent_path() / doodle_config::ue4_config, l_local_ue_project_dir / doodle_config::ue4_config,
      logger_ptr_
  );
  FSys::copy_diff(l_ue_project, l_local_ue_project, logger_ptr_);

  logger_ptr_->warn("排队导入skin文件 {} ", l_local_ue_project);
  auto l_time_info = std::make_shared<server_task_info::run_time_info_t>();
  co_await async_run_ue(
      {l_local_ue_project.generic_string(), "-windowed", "-log", "-stdout", "-AllowStdOutLogVerbosity",
       "-ForceLogFlush", "-Unattended", "-run=DoodleAutoAnimation", fmt::format("-ImportRig={}", l_tmp_path)},
      logger_ptr_, false, l_time_info
  );
  l_time_info->info_ = fmt::format("导入skin文件 {}", l_fbx);
  on_run_time_info_(*l_time_info);

  // 上传文件
  logger_ptr_->warn(
      "上传文件 {} 到项目 {}", l_local_ue_project_dir / doodle_config::ue4_content,
      l_ue_project.parent_path() / doodle_config::ue4_content
  );
  FSys::copy_diff(
      l_local_ue_project_dir / doodle_config::ue4_content, l_ue_project.parent_path() / doodle_config::ue4_content,
      logger_ptr_
  );
  co_return;
}

FSys::path export_rig_sk_arg::get_result() const { return result_path_; }
}  // namespace doodle