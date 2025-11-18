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
#include <spdlog/spdlog.h>

namespace doodle {

boost::asio::awaitable<void> export_rig_sk_arg::run() {
  {
    auto l_json = co_await kitsu_client_->get_generate_uesk_file_arg(task_id_);
    l_json.get_to(impl_);
  }
  auto l_arg = std::make_shared<maya_exe_ns::export_rig_arg>();
  l_arg->set_file_path(maya_file_);
  l_arg->set_logger(logger_ptr_);
  co_await l_arg->async_run_maya();
  auto l_maya_file = l_arg->get_out_arg();

  for (auto&& p : impl_.ue_asset_copy_path_) {
    logger_ptr_->info("复制UE资源文件 from {} to {}", p.from_, p.to_);
    FSys::copy_diff(p.from_, p.to_, logger_ptr_);
  }

  if (l_maya_file.out_file_list.empty()) throw_exception(doodle_error{"文件 {}, 未能输出骨架fbx", maya_file_});

  for (auto& p : l_maya_file.out_file_list) {
    SPDLOG_INFO("导出 {}", p);

    nlohmann::json l_json{};
    l_json =
        import_and_render_ue_ns::import_skin_file{.fbx_file_ = p, .import_dir_ = impl_.import_game_path_.parent_path()};
    auto l_tmp_path   = FSys::write_tmp_file("ue_import", l_json.dump(), ".json");

    auto l_ue_project = ue_exe_ns::find_ue_project_file(impl_.ue_project_path_);
    if (l_ue_project.empty()) throw doodle_error{"无法找到UE项目文件 {}", impl_.ue_project_path_};

    logger_ptr_->warn("排队导入skin文件 {} {}", l_ue_project, p);
    auto l_time_info = std::make_shared<server_task_info::run_time_info_t>();
    co_await async_run_ue(
        {l_ue_project.generic_string(), "-windowed", "-log", "-stdout", "-AllowStdOutLogVerbosity", "-ForceLogFlush",
         "-Unattended", "-run=DoodleAutoAnimation", fmt::format("-ImportRig={}", l_tmp_path)},
        logger_ptr_, true, l_time_info
    );
    l_time_info->info_ = fmt::format("导入skin文件 {}", p);
    on_run_time_info_(*l_time_info);
  }
  // 上传文件
  logger_ptr_->warn("上传文件 {} ", impl_.update_ue_path_);
  co_await kitsu_client_->upload_asset_file_maya(task_id_, maya_file_);
  co_await kitsu_client_->upload_asset_file_ue(
      task_id_, std::make_shared<std::vector<FSys::path>>(std::vector<FSys::path>{impl_.update_ue_path_})
  );

  co_return;
}

}  // namespace doodle