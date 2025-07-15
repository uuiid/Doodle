//
// Created by TD on 25-3-27.
//

#include "doodle_core/metadata/comment.h"
#include "doodle_core/metadata/entity.h"
#include "doodle_core/metadata/preview_file.h"
#include "doodle_core/metadata/task.h"
#include "doodle_core/sqlite_orm/sqlite_database.h"

#include "kitsu.h"
#include "kitsu_reg_url.h"
#include <opencv2/opencv.hpp>
namespace doodle::http {
boost::asio::awaitable<boost::beast::http::message_generator> task_comment_add_preview_post::callback_arg(
    session_data_ptr in_handle, std::shared_ptr<task_comment_add_preview_arg> in_arg
) {
  auto l_person = get_person(in_handle);
  l_person->check_task_action_access(in_arg->task_id);
  auto l_sql      = g_ctx().get<sqlite_database>();

  auto l_comment  = l_sql.get_by_uuid<comment>(in_arg->comment_id);
  auto l_task     = l_sql.get_by_uuid<task>(in_arg->task_id);
  auto l_revision = in_handle->get_json().value("revision", 0);
  if (l_revision == 0 && !l_sql.has_preview_file(in_arg->comment_id))
    l_revision = l_sql.get_next_preview_revision(in_arg->task_id);
  else if (l_revision == 0)
    l_revision = l_sql.get_preview_revision(in_arg->comment_id);
  auto l_position                  = l_sql.get_next_position(in_arg->task_id, l_revision);

  auto l_preview_file              = std::make_shared<preview_file>();
  l_preview_file->uuid_id_         = core_set::get_set().get_uuid();
  l_preview_file->revision_        = l_revision;
  l_preview_file->task_id_         = in_arg->task_id;
  l_preview_file->person_id_       = l_person->person_.uuid_id_;
  l_preview_file->position_        = l_position;
  l_preview_file->name_            = fmt::to_string(l_preview_file->uuid_id_).substr(0, 13);
  l_preview_file->status_          = preview_file_statuses::processing;
  l_preview_file->source_          = "webgui";
  l_preview_file->extension_       = "mp4";
  l_preview_file->created_at_      = chrono::system_clock::now();
  l_preview_file->updated_at_      = chrono::system_clock::now();
  auto l_preview_link              = std::make_shared<comment_preview_link>();
  l_preview_link->comment_id_      = in_arg->comment_id;
  l_preview_link->preview_file_id_ = l_preview_file->uuid_id_;

  co_await l_sql.install(l_preview_file);
  co_await l_sql.install(l_preview_link);
  // 产生事件( "preview-file:new", "comment:update")

  co_return in_handle->make_msg(nlohmann::json{} = *l_preview_file);
}

namespace {
/// 图片扩展名
bool is_image_extension(const FSys::path& in_ext) {
  static const std::set<FSys::path> l_image_exts{".png", ".jpg", ".jpeg"};
  return l_image_exts.contains(in_ext);
}
/// 将图片转换为png图片
FSys::path convert_to_png(const FSys::path& in_path) {
  auto l_new_path = in_path;
  if (l_new_path.extension() != ".png") {
    l_new_path.replace_extension(".png");
    auto l_cv = cv::imread(in_path.generic_string());
    if (l_cv.empty()) {
      throw_exception(doodle_error{"无法读取图片文件: " + in_path.generic_string()});
    }
    cv::imwrite(l_new_path.generic_string(), l_cv);
  }
  return l_new_path;
}
std::tuple<std::size_t, std::size_t> get_image_size(const FSys::path& in_path) {
  auto l_cv = cv::imread(in_path.generic_string());
  if (l_cv.empty()) {
    throw_exception(doodle_error{"无法读取图片文件: " + in_path.generic_string()});
  }
  return {l_cv.cols, l_cv.rows};
}
void save_variants(const FSys::path& in_path, const uuid& in_id) {
  auto l_cv = cv::imread(in_path.generic_string());
  static std::array g_variants{
      std::tuple{"thumbnails", std::make_pair(150, 100)}, std::tuple{"thumbnails_square", std::make_pair(100, 100)},
      std::tuple{"previews", std::make_pair(1200, 0)}
  };
  for (auto&& [key, size] : g_variants) {
    auto l_new_path =
        g_ctx().get<kitsu_ctx_t>().root_ / "pictures" / key / FSys::split_uuid_path(fmt::format("{}.png", in_id));
    if (auto l_p = l_new_path.parent_path(); !FSys::exists(l_p)) FSys::create_directories(l_p);
    auto l_new_cv = l_cv.clone();
    cv::resize(
        l_new_cv, l_new_cv, cv::Size{size.first, size.second == 0 ? (l_cv.cols / l_cv.rows) * size.first : size.second},
        0, 0
    );
    cv::imwrite(l_new_path.generic_string(), l_new_cv);
  }
}

}  // namespace

boost::asio::awaitable<boost::beast::http::message_generator> pictures_preview_files_post::callback_arg(
    session_data_ptr in_handle, std::shared_ptr<capture_id_t> in_arg
) {
  auto l_sql          = g_ctx().get<sqlite_database>();
  auto l_preview_file = std::make_shared<preview_file>(l_sql.get_by_uuid<preview_file>(in_arg->id_));
  if (!l_preview_file->original_name_.empty())
    throw_exception(
        http_request_error{
            boost::beast::http::status::bad_request, "不允许重复上传预览文件, 请删除原有预览文件后再上传新的预览文件"
        }
    );
  auto l_person = get_person(in_handle);
  l_person->check_task_action_access(l_preview_file->task_id_);
  FSys::path l_file;
  if (auto l_fs = in_handle->get_files(); l_fs.empty())
    throw_exception(http_request_error{boost::beast::http::status::bad_request, "请上传预览文件"});
  else
    l_file = l_fs[0];
  if (auto l_ext = l_file.extension(); is_image_extension(l_ext)) {
    save_variants(l_file, l_preview_file->uuid_id_);
    /// 将原始文件添加到相应的位置中
    auto l_new_path = g_ctx().get<kitsu_ctx_t>().root_ / "pictures" / "original" /
                      FSys::split_uuid_path(fmt::format("{}.png", l_preview_file->uuid_id_));
    if (auto l_p = l_new_path.parent_path(); !exists(l_p)) FSys::create_directories(l_p);
    FSys::rename(l_file, l_new_path);
    l_preview_file->extension_     = "png";
    l_preview_file->original_name_ = l_file.stem().generic_string();
    l_file                         = l_new_path;
  }
  auto l_size                 = get_image_size(l_file);
  l_preview_file->width_      = static_cast<std::int32_t>(std::get<0>(l_size));
  l_preview_file->height_     = static_cast<std::int32_t>(std::get<1>(l_size));
  l_preview_file->status_     = preview_file_statuses::ready;
  l_preview_file->file_size_  = FSys::file_size(l_file);
  l_preview_file->updated_at_ = chrono::system_clock::now();
  co_await l_sql.install(l_preview_file);

  // 更新task
  if (l_preview_file->position_ == 1) {
    auto l_task                   = std::make_shared<task>(l_sql.get_by_uuid<task>(l_preview_file->task_id_));
    l_task->last_preview_file_id_ = l_preview_file->uuid_id_;
    co_await l_sql.install(l_task);
    if (auto l_prj = l_sql.get_by_uuid<project>(l_task->project_id_); l_prj.is_set_preview_automated_) {
      auto l_entity              = std::make_shared<entity>(l_sql.get_by_uuid<entity>(l_task->entity_id_));
      l_entity->preview_file_id_ = l_preview_file->uuid_id_;
      // 发送事件 "preview-file:set-main"
      co_await l_sql.install(l_entity);
    }
  }
  co_return in_handle->make_msg(nlohmann::json{} = *l_preview_file);
}
boost::asio::awaitable<boost::beast::http::message_generator> actions_preview_files_set_main_preview_put::callback_arg(
    session_data_ptr in_handle, std::shared_ptr<capture_id_t> in_arg
) {
  auto l_person       = get_person(in_handle);
  auto l_sql          = g_ctx().get<sqlite_database>();
  auto l_preview_file = l_sql.get_by_uuid<preview_file>(in_arg->id_);

  std::int32_t l_frame_number;
  if (auto l_json = in_handle->get_json(); l_json.contains("frame_number") && l_json["frame_number"].is_number()) {
    l_frame_number = l_json["frame_number"].get<std::int32_t>();
  }
  auto l_task = l_sql.get_by_uuid<task>(l_preview_file.task_id_);
  auto l_ent  = std::make_shared<entity>(l_sql.get_by_uuid<entity>(l_task.entity_id_));
  if (l_preview_file.extension_ == "mp4") {
    throw_exception(http_request_error{boost::beast::http::status::bad_request, "mp4文件不支持设置为主预览文件"});
  } else {
    l_ent->preview_file_id_ = l_preview_file.uuid_id_;
    co_await l_sql.install(l_ent);
    // 发送事件 "preview-file:set-main"
  }
  co_return in_handle->make_msg(nlohmann::json{} = *l_ent);
}

}  // namespace doodle::http
