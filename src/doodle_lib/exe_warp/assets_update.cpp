#include "assets_update.h"

#include "http_client/kitsu_client.h"
#include <filesystem>
#include <vector>

namespace doodle {
boost::asio::awaitable<void> update_ue_files::run() {
  std::vector<kitsu::kitsu_client::update_file_arg> l_files{};
  auto l_json = co_await kitsu_client_->get_task_assets_update_ue_files(task_id_);
  l_files     = kitsu::kitsu_client::update_file_arg::list_all_project_files(
      ue_project_path_, l_json.get<std::vector<FSys::path>>()
  );

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