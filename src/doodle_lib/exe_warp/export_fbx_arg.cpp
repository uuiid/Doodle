#include "export_fbx_arg.h"

#include "doodle_core/core/file_sys.h"
#include "doodle_core/metadata/image_size.h"
#include "doodle_core/metadata/move_create.h"
#include "doodle_core/metadata/task_status.h"
#include "doodle_core/metadata/task_type.h"

#include <doodle_lib/long_task/image_to_move.h>

#include <array>
#include <filesystem>
#include <string_view>

namespace doodle {
void from_json(const nlohmann::json& in_json, export_fbx_arg& out_obj) {
  from_json(in_json, static_cast<maya_exe_ns::arg&>(out_obj));

  if (in_json.contains("create_play_blast")) in_json.at("create_play_blast").get_to(out_obj.create_play_blast_);
  if (in_json.contains("camera_film_aperture")) in_json.at("camera_film_aperture").get_to(out_obj.film_aperture_);
  if (in_json.contains("image_size")) in_json.at("image_size").get_to(out_obj.size_);
  if (in_json.contains("only_upload")) in_json.at("only_upload").get_to(out_obj.only_upload_);
  if (in_json.contains("path")) in_json.at("path").get_to(out_obj.maya_file_);
}
// to json
void to_json(nlohmann::json& in_json, const export_fbx_arg& out_obj) {
  to_json(in_json, static_cast<const maya_exe_ns::arg&>(out_obj));

  in_json["create_play_blast"]    = out_obj.create_play_blast_;
  in_json["camera_film_aperture"] = out_obj.film_aperture_;
  in_json["image_size"]           = out_obj.size_;
  in_json["only_upload"]          = out_obj.only_upload_;
}

boost::asio::awaitable<void> export_fbx_arg::run() {
  kitsu_client_->set_logger(logger_ptr_);

  get_export_fbx_arg l_out_arg_{};
  {
    auto l_args = co_await kitsu_client_->get_export_anim_fbx(task_id_);
    l_args.get_to(l_out_arg_);
    film_aperture_ = l_out_arg_.film_aperture_;
    size_          = l_out_arg_.size_;
  }
  auto l_root_dir = maya_file_.parent_path().parent_path();
  constexpr static std::array<std::string_view, 3> g_movie_ext{".mp4", ".mov", ".avi"};
  if (only_upload_) {
    SPDLOG_LOGGER_INFO(logger_ptr_, "仅上传文件 {}", maya_file_);
    auto l_path = l_root_dir / "mov" / l_out_arg_.movie_file_.filename();

    for (auto& ext : g_movie_ext) {
      l_path.replace_extension(ext);
      if (FSys::exists(l_path)) {
        l_out_arg_.movie_file_ = l_path;
        break;
      }
    }
    if (!FSys::exists(l_out_arg_.movie_file_)) l_out_arg_.movie_file_.clear();

    out_arg_.out_file_list = FSys::list_files(l_root_dir / "fbx" / maya_file_.stem(), ".fbx");
  } else {
    co_await arg::async_run_maya();
    if (!out_arg_.movie_file_dir.empty()) {
      auto l_path = l_root_dir / "mov" / l_out_arg_.movie_file_.filename();
      SPDLOG_LOGGER_INFO(logger_ptr_, "导出排屏目录 {} 合成路径 {}", out_arg_.movie_file_dir, l_path);
      if (auto l_p = l_path.parent_path(); !FSys::exists(l_p)) {
        FSys::create_directories(l_p);
      }
      l_out_arg_.movie_file_ = l_path;
      detail::create_move(
          l_out_arg_.movie_file_, logger_ptr_,
          movie::image_attr::make_default_attr(FSys::list_files(out_arg_.movie_file_dir, ".png")), l_out_arg_.size_
      );
    } else {
      l_out_arg_.movie_file_.clear();
    }
  }

  co_await kitsu_client_->upload_shot_animation_maya(task_id_, maya_file_);
  co_await kitsu_client_->remove_shot_animation_export_file(task_id_);
  for (auto& l_p : out_arg_.out_file_list) {
    SPDLOG_LOGGER_INFO(logger_ptr_, "上传导出文件 {}", l_p);
    co_await kitsu_client_->upload_shot_animation_export_file(task_id_, l_p.parent_path(), l_p.filename());
  }
  co_await kitsu_client_->comment_task(
      kitsu::kitsu_client::comment_task_arg{
          .task_id_        = task_id_,
          .comment_        = "自动导出和上传文件",
          .attach_files_   = l_out_arg_.movie_file_,
          .task_status_id_ = task_status::get_nearly_completed(),
      }
  );
  if (!l_out_arg_.movie_file_.empty()) {
    co_await kitsu_client_->upload_shot_animation_other_file(
        task_id_, l_root_dir, l_out_arg_.movie_file_.lexically_proximate(l_root_dir)
    );
  }
}

void from_json(const nlohmann::json& in_json, export_fbx_arg_epiboly& out_obj) {
  from_json(in_json, static_cast<maya_exe_ns::arg&>(out_obj));
}
// to json
void to_json(nlohmann::json& in_json, const export_fbx_arg_epiboly& out_obj) {
  to_json(in_json, static_cast<const maya_exe_ns::arg&>(out_obj));
  in_json["create_play_blast"]    = out_obj.create_play_blast_;
  in_json["camera_film_aperture"] = out_obj.film_aperture_;
  in_json["image_size"]           = out_obj.size_;
}

boost::asio::awaitable<void> export_fbx_arg_epiboly::run() {
  co_await arg::async_run_maya();
  auto l_root_dir = file_path.parent_path().parent_path();
  if (!out_arg_.movie_file_dir.empty()) {
    auto l_path = l_root_dir / "mov" / file_path.stem().concat(".mp4");
    SPDLOG_LOGGER_INFO(logger_ptr_, "导出排屏目录 {} 合成路径 {}", out_arg_.movie_file_dir, l_path);
    if (auto l_p = l_path.parent_path(); !FSys::exists(l_p)) {
      FSys::create_directories(l_p);
    }
    detail::create_move(
        l_path, logger_ptr_, movie::image_attr::make_default_attr(FSys::list_files(out_arg_.movie_file_dir, ".png")),
        size_
    );
  }
}

}  // namespace doodle