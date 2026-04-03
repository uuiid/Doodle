#include "export_fbx_arg.h"

#include "doodle_core/exception/exception.h"
#include "doodle_core/metadata/server_task_info.h"
#include <doodle_core/metadata/image_size.h>
#include <doodle_core/metadata/move_create.h>
#include <doodle_core/metadata/task_status.h>
#include <doodle_core/metadata/task_type.h>

#include <doodle_lib/core/file_sys.h>
#include <doodle_lib/long_task/image_to_move.h>

#include <array>
#include <filesystem>
#include <nlohmann/json_fwd.hpp>
#include <string_view>
#include <vector>

namespace doodle {
void from_json(const nlohmann::json& in_json, export_fbx_arg& out_obj) {
  from_json(in_json, static_cast<maya_exe_ns::arg&>(out_obj));

  if (in_json.contains("create_play_blast")) in_json.at("create_play_blast").get_to(out_obj.create_play_blast_);
  if (in_json.contains("camera_film_aperture")) in_json.at("camera_film_aperture").get_to(out_obj.film_aperture_);
  if (in_json.contains("image_size")) in_json.at("image_size").get_to(out_obj.size_);
  if (in_json.contains("only_upload")) in_json.at("only_upload").get_to(out_obj.only_upload_);
  if (in_json.contains("path")) in_json.at("path").get_to(out_obj.maya_file_);

  if (in_json.contains("frame_in")) in_json.at("frame_in").get_to(out_obj.frame_in_);
  if (in_json.contains("frame_out")) in_json.at("frame_out").get_to(out_obj.frame_out_);
}
// to json
void to_json(nlohmann::json& in_json, const export_fbx_arg& out_obj) {
  to_json(in_json, static_cast<const maya_exe_ns::arg&>(out_obj));

  in_json["create_play_blast"]    = out_obj.create_play_blast_;
  in_json["camera_film_aperture"] = out_obj.film_aperture_;
  in_json["image_size"]           = out_obj.size_;
  in_json["only_upload"]          = out_obj.only_upload_;
  in_json["frame_in"]             = out_obj.frame_in_;
  in_json["frame_out"]            = out_obj.frame_out_;
}

void from_json(const nlohmann::json& j, export_fbx_arg_distributed::args& p) {
  if (j.contains("create_play_blast")) j.at("create_play_blast").get_to(p.create_play_blast_);
  if (j.contains("camera_film_aperture")) j.at("camera_film_aperture").get_to(p.film_aperture_);
  if (j.contains("image_size")) j.at("image_size").get_to(p.size_);
  if (j.contains("frame_in")) j.at("frame_in").get_to(p.frame_in_);
  if (j.contains("frame_out")) j.at("frame_out").get_to(p.frame_out_);
  if (j.contains("maya_file_name")) j.at("maya_file_name").get_to(p.maya_file_name_);
  if (j.contains("task_id")) j.at("task_id").get_to(p.task_id_);
  if (j.contains("download_file")) j.at("download_file").get_to(p.download_file_);
}

void to_json(nlohmann::json& j, const export_fbx_arg_distributed::args& p) {
  j["create_play_blast"]    = p.create_play_blast_;
  j["camera_film_aperture"] = p.film_aperture_;
  j["image_size"]           = p.size_;
  j["frame_in"]             = p.frame_in_;
  j["frame_out"]            = p.frame_out_;
  j["maya_file_name"]       = p.maya_file_name_;
  j["task_id"]              = p.task_id_;
  j["download_file"]        = p.download_file_;
}

void from_json(const nlohmann::json& j, export_fbx_arg_distributed& p) {
  from_json(j, static_cast<maya_exe_ns::arg&>(p));
  j.get_to(p.arg_);
}
void to_json(nlohmann::json& j, const export_fbx_arg_distributed& p) {
  j = p.arg_;
  to_json(j, static_cast<const maya_exe_ns::arg&>(p));
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

boost::asio::awaitable<void> export_fbx_arg::run() {
  kitsu_client_->set_logger(logger_ptr_);

  get_export_fbx_arg l_out_arg_{};
  {
    auto l_args = co_await kitsu_client_->get_export_anim_fbx(task_id_);
    l_args.get_to(l_out_arg_);
    film_aperture_ = l_out_arg_.film_aperture_;
    size_          = l_out_arg_.size_;
    frame_in_      = l_out_arg_.frame_in_;
    frame_out_     = l_out_arg_.frame_out_;
  }

  // 检测孪生文件 LQ_EP001_SC0010.ma 和 LQ_EP001G_SC0010.ma, 他们被视为同一个文件, 需要将  LQ_EP001_SC0010.ma 重命名为
  // LQ_EP001G_SC0010.ma 以便后续正确生成排屏文件和上传
  bool l_fix_twin_file = false;
  {
    std::vector<std::string> l_name_vec{};
    boost::split(l_name_vec, maya_file_.filename().generic_string(), boost::is_any_of("_"));
    if (l_name_vec.size() >= 3) {
      auto l_name_1 = fmt::format("{}_{}_{}", l_name_vec[0], l_name_vec[1], l_name_vec[2]);
      auto l_name_2 = fmt::format("{}_{}G_{}", l_name_vec[0], l_name_vec[1], l_name_vec[2]);
      if (maya_file_.filename().generic_string() == l_name_1 && l_out_arg_.maya_file_name_ == l_name_2) {
        auto l_new_path = maya_file_.parent_path() / "GD" / l_name_2;
        if (FSys::exists(l_new_path)) FSys::remove(l_new_path);
        if (auto parent_path = l_new_path.parent_path(); !FSys::exists(parent_path))
          FSys::create_directories(parent_path);

        FSys::copy(maya_file_, l_new_path);
        maya_file_ = l_new_path;
        file_path  = maya_file_;
        logger_ptr_->info("检测到孪生文件 {}, 已重命名为 {}", l_name_1, l_name_2);
        l_fix_twin_file = true;
      }
    }
  }

  DOODLE_CHICK(
      l_out_arg_.maya_file_name_ == maya_file_.filename(), "Maya文件命名不规范，无法继续后续操作 {} {}",
      l_out_arg_.maya_file_name_, maya_file_.filename()
  );

  auto l_name_end = fmt::format("_{}-{}", frame_in_, frame_out_);
  auto l_root_dir = maya_file_.parent_path().parent_path();
  constexpr static std::array<std::string_view, 3> g_movie_ext{".mp4", ".mov", ".avi"};
  if (only_upload_) {
    // 如果只是上传, 则生成文件遵循之前的命名规范, 不区分孪生文件, 以便后续正确查找排屏文件和上传
    if (l_fix_twin_file) l_root_dir = l_root_dir.parent_path();
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
  DOODLE_CHICK(!out_arg_.out_file_list.empty(), "没有生成任何文件，无法上传");
  for (auto& l_p : out_arg_.out_file_list) {
    SPDLOG_LOGGER_INFO(logger_ptr_, "导出文件 {}", l_p);
    DOODLE_CHICK(
        l_p.stem().string().ends_with(l_name_end), "导出文件 {} 命名不符合规范，无法上传, 名称应以 {} 结尾", l_p,
        l_name_end
    );
  }

  co_await kitsu_client_->upload_shot_animation_maya(task_id_, maya_file_);
  co_await kitsu_client_->remove_shot_animation_export_file(task_id_);
  for (auto& l_p : out_arg_.out_file_list) {
    SPDLOG_LOGGER_INFO(logger_ptr_, "上传导出文件 {}", l_p);
    co_await kitsu_client_->upload_shot_animation_export_file(task_id_, l_p.parent_path(), l_p.filename());
  }
  co_await kitsu_client_->comment_task(
      kitsu::kitsu_client::comment_task_arg{
          .task_id_ = task_id_,
          .comment_ = fmt::format(
              "自动导出和上传文件 拍屏 {}, 只上传 {}", create_play_blast_ ? "是" : "否", only_upload_ ? "是" : "否"
          ),
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

boost::asio::awaitable<void> export_fbx_arg_distributed::run() {
  logger_ptr_         = create_logger();
  auto l_kitsu_client = create_kitsu_client();
  l_kitsu_client->set_logger(logger_ptr_);
  task_info_.command_.get_to(arg_);
  std::string l_error_msg;
  try {
    // 将文件复制到本地路径, 避免网络文件访问导致的 maya 导出失败
    auto l_local_maya_file = core_set::get_set().get_cache_root("temp/export_fbx") / arg_.maya_file_name_;
    if (FSys::exists(l_local_maya_file)) FSys::remove(l_local_maya_file);
    if (auto parent_path = l_local_maya_file.parent_path(); !FSys::exists(parent_path))
      FSys::create_directories(parent_path);
    FSys::copy(arg_.download_file_, l_local_maya_file);
    file_path = l_local_maya_file;

    DOODLE_CHICK(
        arg_.maya_file_name_ == file_path.filename(), "Maya文件命名不规范，无法继续后续操作 {} {}",
        arg_.maya_file_name_, file_path.filename()
    );

    auto l_name_end = fmt::format("_{}-{}", arg_.frame_in_, arg_.frame_out_);
    co_await arg::async_run_maya();
    auto l_root_dir   = file_path.parent_path().parent_path();
    auto l_movie_path = l_root_dir / "mov" / file_path.stem().concat(".mp4");
    if (!out_arg_.movie_file_dir.empty()) {
      SPDLOG_LOGGER_INFO(logger_ptr_, "导出排屏目录 {} 合成路径 {}", out_arg_.movie_file_dir, l_movie_path);
      if (auto l_p = l_movie_path.parent_path(); !FSys::exists(l_p)) {
        FSys::create_directories(l_p);
      }
      detail::create_move(
          l_movie_path, logger_ptr_,
          movie::image_attr::make_default_attr(FSys::list_files(out_arg_.movie_file_dir, ".png")), arg_.size_
      );
    }
    DOODLE_CHICK(!out_arg_.out_file_list.empty(), "没有生成任何文件，无法上传");

    for (auto& l_p : out_arg_.out_file_list) {
      SPDLOG_LOGGER_INFO(logger_ptr_, "导出文件 {}", l_p);
      DOODLE_CHICK(
          l_p.stem().string().ends_with(l_name_end), "导出文件 {} 命名不符合规范，无法上传, 名称应以 {} 结尾", l_p,
          l_name_end
      );
    }

    co_await l_kitsu_client->upload_shot_animation_maya(arg_.task_id_, file_path);
    co_await l_kitsu_client->remove_shot_animation_export_file(arg_.task_id_);
    for (auto& l_p : out_arg_.out_file_list) {
      SPDLOG_LOGGER_INFO(logger_ptr_, "上传导出文件 {}", l_p);
      co_await l_kitsu_client->upload_shot_animation_export_file(arg_.task_id_, l_p.parent_path(), l_p.filename());
    }
    co_await l_kitsu_client->comment_task(
        kitsu::kitsu_client::comment_task_arg{
            .task_id_        = arg_.task_id_,
            .comment_        = fmt::format("自动导出和上传文件 拍屏 {}", arg_.create_play_blast_ ? "是" : "否"),
            .attach_files_   = FSys::exists(l_movie_path) ? l_movie_path : FSys::path{},
            .task_status_id_ = task_status::get_nearly_completed(),
        }
    );
    if (FSys::exists(l_movie_path)) {
      co_await l_kitsu_client->upload_shot_animation_other_file(
          arg_.task_id_, l_root_dir, l_movie_path.lexically_proximate(l_root_dir)
      );
    }
  } catch (...) {
    l_error_msg = fmt::format("导出FBX失败: {}", boost::current_exception_diagnostic_information());
  }
  task_info_.status_   = l_error_msg.empty() ? server_task_info_status::completed : server_task_info_status::failed;
  task_info_.end_time_ = std::chrono::system_clock::now();

  if (!l_error_msg.empty()) logger_ptr_->error(l_error_msg);
  co_await l_kitsu_client->put_job_info(task_info_.uuid_id_, nlohmann::json{} = task_info_);
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