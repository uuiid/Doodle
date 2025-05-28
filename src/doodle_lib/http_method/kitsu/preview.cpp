//
// Created by TD on 25-3-27.
//

#include "doodle_core/metadata/comment.h"
#include "doodle_core/metadata/preview_file.h"
#include "doodle_core/metadata/task.h"
#include "doodle_core/sqlite_orm/sqlite_database.h"

#include "kitsu.h"
#include "kitsu_reg_url.h"
namespace doodle::http {
boost::asio::awaitable<boost::beast::http::message_generator> task_comment_add_preview_post::callback(
    session_data_ptr in_handle
) {
  auto l_person     = get_person(in_handle);
  auto l_task_id    = in_handle->capture_->get_uuid("task_id");
  auto l_comment_id = in_handle->capture_->get_uuid("comment_id");
  l_person->check_task_action_access(l_task_id);
  auto l_sql                       = g_ctx().get<sqlite_database>();

  auto l_comment                   = l_sql.get_by_uuid<comment>(l_comment_id);
  auto l_task                      = l_sql.get_by_uuid<task>(l_task_id);
  auto l_revision                  = in_handle->get_json().value("revision", 0);
  auto l_position                  = l_revision == 0
                                         ? (l_sql.has_preview_file(l_task_id) ? l_sql.get_next_position(l_task_id, l_revision)
                                                                              : l_sql.get_next_preview_revision(l_task_id))
                                         : l_sql.get_next_position(l_task_id, l_revision);
  auto l_preview_file              = std::make_shared<preview_file>();
  l_preview_file->uuid_id_         = core_set::get_set().get_uuid();
  l_preview_file->revision_        = l_revision;
  l_preview_file->task_id_         = l_task_id;
  l_preview_file->person_id_       = l_person->person_.uuid_id_;
  l_preview_file->position_        = l_position;
  l_preview_file->name_            = fmt::to_string(l_preview_file->uuid_id_).substr(0, 13);
  l_preview_file->status_          = preview_file_statuses::processing;
  l_preview_file->source_          = "webgui";
  l_preview_file->extension_       = "mp4";
  l_preview_file->created_at_      = chrono::system_clock::now();
  l_preview_file->updated_at_      = chrono::system_clock::now();
  auto l_preview_link              = std::make_shared<comment_preview_link>();
  l_preview_link->comment_id_      = l_comment_id;
  l_preview_link->preview_file_id_ = l_preview_file->uuid_id_;

  co_await l_sql.install(l_preview_file);
  co_await l_sql.install(l_preview_link);
  // 产生事件( "preview-file:new", "comment:update")

  co_return in_handle->make_msg(nlohmann::json{} = *l_preview_file);
}

}  // namespace doodle::http