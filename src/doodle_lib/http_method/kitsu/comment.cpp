//
// Created by TD on 25-4-29.
//
#include <doodle_core/metadata/comment.h>
#include <doodle_core/metadata/notification.h>
#include <doodle_core/metadata/status_automation.h>
#include <doodle_core/metadata/task.h>
#include <doodle_core/metadata/task_status.h>
#include <doodle_core/sqlite_orm/sqlite_database.h>

#include <doodle_lib/http_method/kitsu/kitsu_reg_url.h>
namespace doodle::http {

boost::asio::awaitable<boost::beast::http::message_generator> task_comment_post::callback(session_data_ptr in_handle) {
  auto l_person                      = get_person(in_handle);
  std::shared_ptr<comment> l_comment = std::make_shared<comment>();
  auto l_json                        = in_handle->get_json();
  l_json.get_to(*l_comment);
  auto l_task_id         = from_uuid_str(in_handle->capture_->get("task_id"));
  l_comment->uuid_id_    = core_set::get_set().get_uuid();
  l_comment->created_at_ = chrono::system_clock::now();
  l_comment->updated_at_ = chrono::system_clock::now();
  l_comment->person_id_  = l_person->person_.uuid_id_;
  l_comment->object_id_  = l_task_id;

  auto l_sql             = g_ctx().get<sqlite_database>();
  auto l_task            = std::make_shared<task>(l_sql.get_by_uuid<task>(l_task_id));
  auto l_task_status     = l_sql.get_by_uuid<task_status>(l_comment->task_status_id_);
  l_task_status.check_retake_capping(*l_task);
  {
    l_comment->set_comment_department_mentions();
    l_comment->set_comment_mentions(l_task->project_id_);
    co_await l_sql.install(l_comment);
    auto l_comment_mentions = std::make_shared<std::vector<comment_mentions>>(
        l_comment->mentions_ | ranges::views::transform([l_comment](const uuid& in) {
          return comment_mentions{.comment_id_ = l_comment->uuid_id_, .person_id_ = in};
        }) |
        ranges::to_vector
    );
    co_await l_sql.install_range(l_comment_mentions);
    auto l_comment_department_mentions = std::make_shared<std::vector<comment_department_mentions>>(
        l_comment->department_mentions_ | ranges::views::transform([l_comment](const uuid& in) {
          return comment_department_mentions{.comment_id_ = l_comment->uuid_id_, .department_id_ = in};
        }) |
        ranges::to_vector
    );
    co_await l_sql.install_range(l_comment_department_mentions);
  }
  bool l_status_changed;
  if (!l_task->last_comment_date_ ||
      l_task->last_comment_date_->get_sys_time() < l_comment->created_at_.get_sys_time()) {
    l_status_changed           = l_task->task_status_id_ != l_comment->task_status_id_;
    l_task->last_comment_date_ = l_comment->created_at_;
    l_task->task_status_id_    = l_comment->task_status_id_;
    if (l_task_status.is_retake_) ++l_task->retake_count_;
    if (l_task_status.is_feedback_request_) l_task->end_date_ = chrono::system_clock::now();
    co_await l_sql.install(l_task);
    if (l_status_changed)  // todo: 需要通知状态改变
      ;
  }
  // 创建通知
  {
    auto l_notification_person = l_sql.get_notification_recipients(*l_task);
    if (l_notification_person.contains(l_person->person_.uuid_id_))
      l_notification_person.erase(l_person->person_.uuid_id_);
    auto l_notifications = std::make_shared<std::vector<notification>>();
    for (auto&& i : l_notification_person) {
      l_notifications->emplace_back(
          notification{
              .uuid_id_    = core_set::get_set().get_uuid(),
              .read_       = false,
              .change_     = l_status_changed,
              .type_       = notification_type::comment,
              .person_id_  = i,
              .author_id_  = l_person->person_.uuid_id_,
              .comment_id_ = l_comment->uuid_id_,
              .task_id_    = l_task->uuid_id_,
          }
      );
    }
    for (auto&& i : l_sql.get_mentioned_people(l_task->project_id_, *l_comment)) {
      l_notifications->emplace_back(
          notification{
              .uuid_id_    = core_set::get_set().get_uuid(),
              .read_       = false,
              .change_     = false,
              .type_       = notification_type::mention,
              .person_id_  = i,
              .author_id_  = l_person->person_.uuid_id_,
              .comment_id_ = l_comment->uuid_id_,
              .task_id_    = l_task->uuid_id_,
          }
      );
    }
    co_await l_sql.install_range(l_notifications);
  }

  {  // 运行自动化任务
    for (auto&& i : l_sql.get_project_status_automations(l_task->project_id_))
      co_await i.run(l_task, l_person->person_.uuid_id_);
  }
  nlohmann::json l_r{};
  l_r                = *l_comment;
  l_r["task_status"] = l_task_status;
  l_r["person"]      = l_person->person_;

  co_return in_handle->make_msg(l_r);
}

boost::asio::awaitable<boost::beast::http::message_generator> data_comment_put::callback(session_data_ptr in_handle) {
  auto l_json       = in_handle->get_json();
  auto l_comment_id = from_uuid_str(in_handle->capture_->get("comment_id"));
  auto l_sql        = g_ctx().get<sqlite_database>();
  auto l_comment    = std::make_shared<comment>(l_sql.get_by_uuid<comment>(l_comment_id));
  l_json.get_to(*l_comment);
  l_comment->updated_at_ = chrono::system_clock::now();
  co_await l_sql.install(l_comment);
  co_return in_handle->make_msg(nlohmann::json{});
}

}  // namespace doodle::http