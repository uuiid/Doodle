#include "assets_update.h"

#include "doodle_core/core/core_set.h"
#include "doodle_core/core/file_sys.h"
#include "doodle_core/exception/exception.h"
#include "doodle_core/metadata/image_size.h"
#include "doodle_core/metadata/move_create.h"
#include "doodle_core/metadata/project.h"
#include "doodle_core/metadata/task_status.h"

#include <doodle_lib/exe_warp/ue_exe.h>
#include <doodle_lib/http_method/kitsu/preview.h>
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
void from_json(const nlohmann::json& in_json, update_movie_compose_files& out_obj) {
  // if (in_json.contains("task_id")) in_json.at("task_id").get_to(out_obj.task_id_);
  if (in_json.contains("path")) in_json.at("path").get_to(out_obj.movie_compose_file_);
}

boost::asio::awaitable<void> update_ue_files::run() {
  std::vector<kitsu::kitsu_client::update_file_arg> l_files{};
  auto l_json           = co_await kitsu_client_->get_task_assets_update_ue_files(task_id_);

  auto l_ue_project_dir = ue_exe_ns::find_ue_project_file(ue_project_path_).parent_path();
  std::vector<FSys::path> l_file_paths{};
  for (auto&& l_path : l_json.get<std::vector<FSys::path>>()) {
    if (l_path.stem() == "a_PropPublicFiles" && !FSys::exists(l_ue_project_dir / l_path)) {
      SPDLOG_LOGGER_WARN(
          logger_ptr_, "公共资源文件 {} 不存在，可能是因为当前项目没有关联公共资源库，直接跳过上传",
          (l_ue_project_dir / l_path).string()
      );
      continue;
    }
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
  DOODLE_CHICK(!movie_file_.empty(), "视频文件路径不能为空");
  DOODLE_CHICK(!task_id_.is_nil(), "任务ID不能为空");

  kitsu_client_->set_logger(logger_ptr_);
  SPDLOG_LOGGER_INFO(logger_ptr_, "发现需要更新的视频文件 {}", movie_file_);

  auto l_prj = (co_await kitsu_client_->get_tasks_full(this->task_id_)).at("project").get<project>();

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
    DOODLE_CHICK(
        detail::get_image_size(l_files.front()) == l_prj.get_resolution(),
        "图片尺寸与项目分辨率不符 图片尺寸 {}x{} 项目分辨率 {}x{}", detail::get_image_size(l_files.front()).width,
        detail::get_image_size(l_files.front()).height, l_prj.get_resolution().first, l_prj.get_resolution().second
    );
    detail::create_move(
        l_movie_file, logger_ptr_, movie::image_attr::make_default_attr(l_files), image_size{l_prj.get_resolution()}
    );
  } else if (FSys::is_regular_file(movie_file_)) {
    l_movie_file       = movie_file_;
    auto l_video_info  = http::preview::get_video_duration(l_movie_file);
    auto l_tag_size    = l_prj.get_resolution();
    auto l_tag_size_cv = cv::Size{l_tag_size.first, l_tag_size.second};

    DOODLE_CHICK(
        l_video_info.size_ == l_tag_size_cv, "视频尺寸与项目分辨率不符 视频尺寸 {}x{} 项目分辨率 {}x{}",
        l_video_info.size_.width, l_video_info.size_.height, l_tag_size.first, l_tag_size.second
    );
  }
  DOODLE_CHICK(FSys::exists(l_movie_file), "视频文件 {} 不存在", l_movie_file.string());
  co_await kitsu_client_->upload_shot_animation_video_file(task_id_, l_movie_file);
  co_await kitsu_client_->comment_task(
      kitsu::kitsu_client::comment_task_arg{
          .task_id_        = task_id_,
          .comment_        = "更新视频文件",
          .attach_files_   = l_movie_file,
          .task_status_id_ = task_status::get_nearly_completed(),
      }
  );
}
boost::asio::awaitable<void> update_movie_compose_files::run() {
  DOODLE_CHICK(!movie_compose_file_.empty(), "视频合成文件路径不能为空");
  DOODLE_CHICK(!task_id_.is_nil(), "任务ID不能为空");

  kitsu_client_->set_logger(logger_ptr_);
  SPDLOG_LOGGER_INFO(logger_ptr_, "发现需要更新的视频合成文件 {}", movie_compose_file_);
  auto l_prj = (co_await kitsu_client_->get_tasks_full(this->task_id_)).at("project").get<project>();

  DOODLE_CHICK(FSys::exists(movie_compose_file_), "视频合成文件 {} 不存在", movie_compose_file_.string());

  auto l_list_file = FSys::list_files(movie_compose_file_, ".png");
  DOODLE_CHICK(!l_list_file.empty(), "视频合成路径 {} 下没有找到图片文件", movie_compose_file_.string());
  DOODLE_CHICK(
      detail::get_image_size(l_list_file.front()) == l_prj.get_resolution(),
      "图片尺寸与项目分辨率不符 图片尺寸 {}x{} 项目分辨率 {}x{}", detail::get_image_size(l_list_file.front()).width,
      detail::get_image_size(l_list_file.front()).height, l_prj.get_resolution().first, l_prj.get_resolution().second
  );

  auto l_movie_path = core_set::get_set().get_cache_root("movie_compose") / movie_compose_file_.filename();
  l_movie_path.replace_extension(".mp4");
  detail::create_move(
      l_movie_path, logger_ptr_, movie::image_attr::make_default_attr(FSys::list_files(movie_compose_file_, ".png")),
      image_size{l_prj.get_resolution()}
  );

  co_await kitsu_client_->comment_task_compose_video(
      kitsu::kitsu_client::comment_task_arg{
          .task_id_             = task_id_,
          .comment_             = "送审特效样片",
          .attach_files_        = l_movie_path,
          .preview_file_source_ = preview_file_source_enum::vfx_review,
      }
  );
}
}  // namespace doodle