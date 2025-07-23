//
// Created by TD on 25-4-29.
//
#include <doodle_core/metadata/attachment_file.h>
#include <doodle_core/metadata/comment.h>
#include <doodle_core/metadata/notification.h>
#include <doodle_core/metadata/status_automation.h>
#include <doodle_core/metadata/task.h>
#include <doodle_core/metadata/task_status.h>
#include <doodle_core/sqlite_orm/sqlite_database.h>

#include <doodle_lib/core/socket_io/broadcast.h>
#include <doodle_lib/http_method/kitsu/kitsu_reg_url.h>

#include "kitsu.h"
namespace doodle::http {
namespace {
struct create_comment_result : comment {
  task_status task_status_;
  person person_;
  std::vector<attachment_file> attachment_file_;
  explicit create_comment_result(
      const comment& in_comment, const task_status& in_task_status, const person& in_person,
      const std::vector<attachment_file>& in_attachment_file
  )
      : comment(in_comment),
        task_status_(in_task_status),
        person_(in_person),
        attachment_file_(in_attachment_file)

  {}

  // to json
  friend void to_json(nlohmann::json& j, const create_comment_result& p) {
    to_json(j, static_cast<const comment&>(p));
    j["task_status"]     = p.task_status_;
    j["person"]          = p.person_;
    j["attachment_file"] = p.attachment_file_;
  }
};
boost::asio::awaitable<create_comment_result> create_comment(
    std::shared_ptr<comment> in_comment, const http_jwt_fun::http_jwt_t* in_person, uuid in_task_id,
    std::vector<FSys::path> in_files
) {
  in_comment->uuid_id_    = core_set::get_set().get_uuid();
  in_comment->created_at_ = chrono::system_clock::now();
  in_comment->updated_at_ = chrono::system_clock::now();
  in_comment->person_id_  = in_person->person_.uuid_id_;
  if (!in_task_id.is_nil()) in_comment->object_id_ = in_task_id;

  auto l_sql         = g_ctx().get<sqlite_database>();
  auto l_task        = std::make_shared<task>(l_sql.get_by_uuid<task>(in_comment->object_id_));
  auto l_task_status = l_sql.get_by_uuid<task_status>(in_comment->task_status_id_);
  l_task_status.check_retake_capping(*l_task);
  std::vector<attachment_file> l_attachment_files{};
  {  // 创建基本的评论(包括辅助结构)
    in_comment->set_comment_department_mentions();
    in_comment->set_comment_mentions(l_task->project_id_);
    co_await l_sql.install(in_comment);
    auto in_comment_mentions = std::make_shared<std::vector<comment_mentions>>(
        in_comment->mentions_ | ranges::views::transform([in_comment](const uuid& in) {
          return comment_mentions{.comment_id_ = in_comment->uuid_id_, .person_id_ = in};
        }) |
        ranges::to_vector
    );
    co_await l_sql.install_range(in_comment_mentions);
    auto in_comment_department_mentions = std::make_shared<std::vector<comment_department_mentions>>(
        in_comment->department_mentions_ | ranges::views::transform([in_comment](const uuid& in) {
          return comment_department_mentions{.comment_id_ = in_comment->uuid_id_, .department_id_ = in};
        }) |
        ranges::to_vector
    );
    co_await l_sql.install_range(in_comment_department_mentions);

    // 创建附属文件
    for (auto&& i : in_files) {
      auto l_attachment_file         = std::make_shared<attachment_file>();
      l_attachment_file->uuid_id_    = core_set::get_set().get_uuid();
      l_attachment_file->comment_id_ = in_comment->uuid_id_;
      auto l_ext                     = i.extension();
      l_attachment_file->extension_  = l_ext.empty() ? std::string{} : l_ext.generic_string().substr(1);
      l_attachment_file->name_       = i.filename().generic_string();
      l_attachment_file->mimetype_   = kitsu::mime_type(i.extension());
      l_attachment_file->size_       = FSys::file_size(i);
      auto l_attachment_file_path    = g_ctx().get<kitsu_ctx_t>().root_ / "files" / "attachments" /
                                    FSys::split_uuid_path(fmt::to_string(l_attachment_file->uuid_id_));
      if (auto l_p = l_attachment_file_path.parent_path(); !exists(l_p)) FSys::create_directories(l_p);
      FSys::rename(
          i, g_ctx().get<kitsu_ctx_t>().root_ / "files" / "attachments" /
                 FSys::split_uuid_path(fmt::to_string(l_attachment_file->uuid_id_))
      );
      co_await l_sql.install(l_attachment_file);
      l_attachment_files.emplace_back(*l_attachment_file);
    }
  }
  // 更改任务状态
  bool l_status_changed;
  if (!l_task->last_comment_date_ ||
      l_task->last_comment_date_->get_sys_time() < in_comment->created_at_.get_sys_time()) {
    l_status_changed           = l_task->task_status_id_ != in_comment->task_status_id_;
    l_task->last_comment_date_ = in_comment->created_at_;
    l_task->task_status_id_    = in_comment->task_status_id_;
    if (l_task_status.is_retake_) ++l_task->retake_count_;
    if (l_task_status.is_feedback_request_) l_task->end_date_ = chrono::system_clock::now();
    co_await l_sql.install(l_task);
    if (l_status_changed)  // todo: 需要通知状态改变
      ;
  }
  // 创建通知
  {
    auto l_notification_person = l_sql.get_notification_recipients(*l_task);
    if (l_notification_person.contains(in_person->person_.uuid_id_))
      l_notification_person.erase(in_person->person_.uuid_id_);
    auto l_notifications = std::make_shared<std::vector<notification>>();
    for (auto&& i : l_notification_person) {
      l_notifications->emplace_back(
          notification{
              .uuid_id_    = core_set::get_set().get_uuid(),
              .read_       = false,
              .change_     = l_status_changed,
              .type_       = notification_type::comment,
              .person_id_  = i,
              .author_id_  = in_person->person_.uuid_id_,
              .comment_id_ = in_comment->uuid_id_,
              .task_id_    = l_task->uuid_id_,
          }
      );
    }
    for (auto&& i : l_sql.get_mentioned_people(l_task->project_id_, *in_comment)) {
      l_notifications->emplace_back(
          notification{
              .uuid_id_    = core_set::get_set().get_uuid(),
              .read_       = false,
              .change_     = false,
              .type_       = notification_type::mention,
              .person_id_  = i,
              .author_id_  = in_person->person_.uuid_id_,
              .comment_id_ = in_comment->uuid_id_,
              .task_id_    = l_task->uuid_id_,
          }
      );
    }
    co_await l_sql.install_range(l_notifications);
  }

  {  // 运行自动化任务
    for (auto&& i : l_sql.get_project_status_automations(l_task->project_id_))
      co_await i.run(l_task, in_person->person_.uuid_id_);
  }

  socket_io::broadcast(
      "comment:new",
      nlohmann::json{
          {"comment_id", in_comment->uuid_id_},
          {"task_id", in_comment->object_id_},
          {"task_status_id", in_comment->task_status_id_},
          {"project_id", l_task->project_id_}
      },
      "/events"
  );

  co_return create_comment_result{*in_comment, l_task_status, in_person->person_, l_attachment_files};
}
}  // namespace
boost::asio::awaitable<boost::beast::http::message_generator> task_comment_post::callback_arg(
    session_data_ptr in_handle, std::shared_ptr<capture_id_t> in_arg
) {
  auto l_person                      = get_person(in_handle);
  std::shared_ptr<comment> l_comment = std::make_shared<comment>();
  auto l_json                        = in_handle->get_json();
  auto l_files                       = in_handle->get_files();
  l_json.get_to(*l_comment);
  auto l_result = co_await create_comment(l_comment, l_person.get(), in_arg->id_, l_files);
  co_return in_handle->make_msg(nlohmann::json{} = l_result);
}

boost::asio::awaitable<boost::beast::http::message_generator> actions_projects_tasks_comment_many_post::callback_arg(
    session_data_ptr in_handle, std::shared_ptr<capture_id_t> in_arg
) {
  auto l_person = get_person(in_handle);
  auto l_sql    = g_ctx().get<sqlite_database>();
  std::vector<create_comment_result> l_result{};
  for (auto&& i : in_handle->get_json()) {
    auto l_comm = std::make_shared<comment>(i.get<comment>());
    l_result.emplace_back(co_await create_comment(l_comm, l_person.get(), {}, {}));
  }
  co_return in_handle->make_msg(nlohmann::json{} = l_result);
}

boost::asio::awaitable<boost::beast::http::message_generator> data_comment_get::callback_arg(
    session_data_ptr in_handle, std::shared_ptr<capture_id_t> in_arg
) {
  auto l_person  = get_person(in_handle);
  auto l_sql     = g_ctx().get<sqlite_database>();
  auto l_comment = l_sql.get_by_uuid<comment>(in_arg->id_);
  co_return in_handle->make_msg(nlohmann::json{} = l_comment);
}

boost::asio::awaitable<boost::beast::http::message_generator> task_comment_delete_::callback_arg(
    session_data_ptr in_handle, std::shared_ptr<task_comment_arg> in_arg
) {
  auto l_sql    = g_ctx().get<sqlite_database>();
  auto l_task   = std::make_shared<task>(l_sql.get_by_uuid<task>(in_arg->task_id_));
  auto l_person = get_person(in_handle);
  l_person->check_delete_access(l_task->project_id_);
  co_await l_sql.remove<comment>(in_arg->comment_id_);
  auto l_last_comment = l_sql.get_last_comment(l_task->uuid_id_);
  if (l_last_comment) {
    auto l_task_status         = l_sql.get_by_uuid<task_status>(l_task->task_status_id_);
    l_task->last_comment_date_ = l_last_comment->created_at_;
    l_task->task_status_id_    = l_last_comment->task_status_id_;
    if (l_task_status.is_feedback_request_) l_task->end_date_ = l_last_comment->created_at_;
    if (l_task_status.is_done_) l_task->done_date_ = l_last_comment->created_at_;
    co_await l_sql.install(l_task);
  }
  co_return in_handle->make_msg(nlohmann::json{});
}

boost::asio::awaitable<boost::beast::http::message_generator> data_comment_put::callback_arg(
    session_data_ptr in_handle, std::shared_ptr<capture_id_t> in_arg
) {
  auto l_json    = in_handle->get_json();
  auto l_sql     = g_ctx().get<sqlite_database>();
  auto l_comment = std::make_shared<comment>(l_sql.get_by_uuid<comment>(in_arg->id_));
  l_json.get_to(*l_comment);
  l_comment->updated_at_ = chrono::system_clock::now();
  co_await l_sql.install(l_comment);
  co_return in_handle->make_msg(nlohmann::json{} = *l_comment);
}

}  // namespace doodle::http