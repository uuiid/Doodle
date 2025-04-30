//
// Created by TD on 25-4-29.
//
#include <doodle_core/metadata/comment.h>
#include <doodle_core/metadata/task.h>
#include <doodle_core/metadata/task_status.h>
#include <doodle_core/sqlite_orm/sqlite_database.h>

#include <doodle_lib/http_method/kitsu/kitsu_reg_url.h>
namespace doodle::http {

boost::asio::awaitable<boost::beast::http::message_generator> task_comment_post::callback(session_data_ptr in_handle) {
  auto l_person = get_person(in_handle);
  comment l_comment{};
  auto l_json = in_handle->get_json();
  l_json.get_to(l_comment);
  auto l_task_id        = from_uuid_str(in_handle->capture_->get("task_id"));
  l_comment.uuid_id_    = core_set::get_set().get_uuid();
  l_comment.created_at_ = chrono::system_clock::now();
  l_comment.updated_at_ = chrono::system_clock::now();
  l_comment.person_id_  = l_person->person_.uuid_id_;
  l_comment.object_id_  = l_task_id;

  auto l_sql            = g_ctx().get<sqlite_database>();

  auto l_task           = l_sql.get_by_uuid<task>(l_task_id);
  auto l_task_status    = l_sql.get_by_uuid<task_status>(l_comment.task_status_id_);
  l_task_status.check_retake_capping(l_task);

  co_return in_handle->make_msg(nlohmann::json{});
}

boost::asio::awaitable<boost::beast::http::message_generator> data_comment_put::callback(session_data_ptr in_handle) {
  co_return in_handle->make_msg(nlohmann::json{});
}

}  // namespace doodle::http