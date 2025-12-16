#include "assets_update.h"

#include "doodle_core/core/file_sys.h"
#include "doodle_core/exception/exception.h"
#include "doodle_core/metadata/task_status.h"

#include <doodle_lib/exe_warp/ue_exe.h>
#include <doodle_lib/long_task/image_to_move.h>

#include "http_client/kitsu_client.h"
#include <array>
#include <filesystem>
#include <string_view>
#include <vector>

namespace doodle {
void from_json(const nlohmann::json& in_json, update_ue_files& out_obj) {
  if (in_json.contains("task_id")) in_json.at("task_id").get_to(out_obj.task_id_);
  if (in_json.contains("path")) in_json.at("path").get_to(out_obj.ue_project_path_);
}

void from_json(const nlohmann::json& in_json, update_movie_files& out_obj) {
  if (in_json.contains("task_id")) in_json.at("task_id").get_to(out_obj.task_id_);
  if (in_json.contains("path")) in_json.at("path").get_to(out_obj.movie_file_);
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
boost::asio::awaitable<void> update_movie_files::run() {
  if (movie_file_.empty()) co_return;
  kitsu_client_->set_logger(logger_ptr_);
  SPDLOG_LOGGER_INFO(logger_ptr_, "发现需要更新的视频文件 {}", movie_file_);

  FSys::path l_movie_file{};

  static constexpr std::array<std::string_view, 3> k_image_exts{".png", ".jpg", ".jpeg"};

  if (FSys::is_directory(movie_file_)) {
    std::vector<FSys::path> l_files{};
    for (auto&& l_ext : k_image_exts)
      if (l_files = FSys::list_files(movie_file_, l_ext); !l_files.empty()) {
        l_movie_file = l_files.front().parent_path() / (movie_file_.filename().string() + ".mp4");
        break;
      }
    DOODLE_CHICK(!l_files.empty(), "无法找到目录下 {} 中的图片文件(扩展名 .png, .jpg, .jpeg)", movie_file_);
    detail::create_move(l_movie_file, logger_ptr_, movie::image_attr::make_default_attr(l_files));
  } else if (FSys::is_regular_file(movie_file_)) {
    l_movie_file = movie_file_;
  }
  DOODLE_CHICK(FSys::exists(l_movie_file), "视频文件 {} 不存在", l_movie_file.string());
  co_await kitsu_client_->upload_shot_animation_video_file(task_id_, l_movie_file);
  co_await kitsu_client_->comment_task(task_id_, "自动上传评论", l_movie_file, task_status::get_completed());
}
}  // namespace doodle