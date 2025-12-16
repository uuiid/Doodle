#include "assets_update.h"

#include <doodle_lib/exe_warp/ue_exe.h>

#include "http_client/kitsu_client.h"
#include <filesystem>
#include <vector>

namespace doodle {
void from_json(const nlohmann::json& in_json, update_ue_files& out_obj) {
  if (in_json.contains("task_id")) in_json.at("task_id").get_to(out_obj.task_id_);
  if (in_json.contains("ue_project_path")) in_json.at("ue_project_path").get_to(out_obj.ue_project_path_);
}
boost::asio::awaitable<void> update_ue_files::run() {
  std::vector<kitsu::kitsu_client::update_file_arg> l_files{};
  auto l_json           = co_await kitsu_client_->get_task_assets_update_ue_files(task_id_);

  auto l_ue_project_dir = ue_exe_ns::find_ue_project_file(ue_project_path_).parent_path();
  std::vector<FSys::path> l_file_paths{};
  for (auto&& l_path : l_json.get<std::vector<FSys::path>>()) {
    SPDLOG_LOGGER_INFO(logger_ptr_, "需要更新的UE文件路径 {}", l_path.string());
    l_file_paths.push_back(l_ue_project_dir / l_path);
  }

  l_files = kitsu::kitsu_client::update_file_arg::list_all_project_files(ue_project_path_, l_file_paths);

  if (l_files.empty()) co_return;
  kitsu_client_->set_logger(logger_ptr_);
  SPDLOG_LOGGER_INFO(logger_ptr_, "发现需要更新的UE文件数量 {}", l_files.size());
  co_await kitsu_client_->upload_asset_file_ue(task_id_, l_files);
}

boost::asio::awaitable<void> update_image_files::run() {
  if (image_files_.empty()) co_return;
  kitsu_client_->set_logger(logger_ptr_);
  SPDLOG_LOGGER_INFO(logger_ptr_, "发现需要更新的图片文件数量 {}", image_files_.size());
  std::vector<kitsu::kitsu_client::update_file_arg> l_files{};
  for (auto&& p : image_files_) {
    co_await kitsu_client_->upload_asset_file_image(task_id_, p);
  }
}

}  // namespace doodle