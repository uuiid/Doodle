//
// Created by TD on 25-4-29.
//
#include "comment.h"

#include "doodle_core/core/chrono_.h"
#include "doodle_core/sqlite_orm/detail/sqlite_database_impl.h"
#include <doodle_core/metadata/attachment_file.h>
#include <doodle_core/metadata/comment.h>
#include <doodle_core/metadata/notification.h>
#include <doodle_core/metadata/status_automation.h>
#include <doodle_core/metadata/task.h>
#include <doodle_core/metadata/task_status.h>
#include <doodle_core/sqlite_orm/sqlite_database.h>

#include <doodle_lib/core/socket_io/broadcast.h>
#include <doodle_lib/http_method/kitsu.h>
#include <doodle_lib/http_method/kitsu/kitsu_reg_url.h>

#include <algorithm>
#include <chrono>
#include <fmt/format.h>
#include <memory>

namespace doodle::http {
boost::asio::awaitable<create_comment_result> create_comment(
    std::shared_ptr<comment> in_comment, const http_jwt_fun::http_jwt_t* in_person, uuid in_task_id,
    std::vector<FSys::path> in_files, std::shared_ptr<task> in_task
) {
  if (!in_comment->text_.empty()) in_comment->text_ = boost::locale::conv::to_utf<char>(in_comment->text_, "UTF-8");

  in_comment->person_id_ = in_person->person_.uuid_id_;

  if (!in_task_id.is_nil()) in_comment->object_id_ = in_task_id;

  auto l_sql  = g_ctx().get<sqlite_database>();
  auto l_task = in_task ? in_task : std::make_shared<task>(l_sql.get_by_uuid<task>(in_comment->object_id_));

  if (in_comment->task_status_id_.is_nil()) in_comment->task_status_id_ = l_task->task_status_id_;

  in_person->check_task_status_access(in_comment->task_status_id_);

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
      l_attachment_file->comment_id_ = in_comment->uuid_id_;
      auto l_ext                     = i.extension();
      l_attachment_file->extension_  = l_ext.empty() ? std::string{} : l_ext.generic_string().substr(1);
      l_attachment_file->name_       = i.filename().generic_string();
      l_attachment_file->mimetype_   = kitsu::mime_type(i.extension());
      l_attachment_file->size_       = FSys::file_size(i);
      co_await l_sql.install(l_attachment_file);
      auto l_attachment_file_path = g_ctx().get<kitsu_ctx_t>().get_attachment_file(l_attachment_file->uuid_id_);
      if (auto l_p = l_attachment_file_path.parent_path(); !exists(l_p)) FSys::create_directories(l_p);
      FSys::rename(i, l_attachment_file_path);
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
    co_await l_sql.update(l_task);

    socket_io::broadcast(
        "task:update", nlohmann::json{{"task_id", in_comment->object_id_}, {"project_id", l_task->project_id_}},
        "/events"
    );

    if (l_status_changed) {
      // 需要通知状态改变
      socket_io::broadcast(
          "task:status-changed",
          nlohmann::json{
              {"task_id", in_comment->object_id_},
              {"new_task_status_id", l_task_status.uuid_id_},
              {"previous_task_status_id", l_task_status.uuid_id_},
              {"person_id", in_comment->person_id_},
              {"project_id", l_task->project_id_}
          },
          "/events"
      );
    };
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

  // 运行自动化任务
  for (auto&& i : l_sql.get_project_status_automations(l_task->project_id_))
    co_await i.run(l_task, in_person->person_.uuid_id_);

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
boost::asio::awaitable<boost::beast::http::message_generator> actions_tasks_comment::post(session_data_ptr in_handle) {
  person_.check_task_action_access(id_);

  SPDLOG_LOGGER_WARN(
      g_logger_ctrl().get_http(), "用户 {}({}) 开始创建任务 {} 评论", person_.person_.email_,
      person_.person_.get_full_name(), id_
  );

  std::shared_ptr<comment> l_comment = std::make_shared<comment>();
  auto l_json                        = in_handle->get_json();
  auto l_files                       = in_handle->get_files();
  l_json.get_to(*l_comment);
  auto l_result = co_await create_comment(l_comment, &person_, id_, l_files);
  default_logger_raw()->info("由 {} 创建评论 {}", person_.person_.email_, l_comment->uuid_id_);

  SPDLOG_LOGGER_WARN(
      g_logger_ctrl().get_http(), "用户 {}({}) 完成创建任务 {} 评论 {} 附件数量 {}", person_.person_.email_,
      person_.person_.get_full_name(), id_, l_comment->uuid_id_, l_files.size()
  );

  co_return in_handle->make_msg(nlohmann::json{} = l_result);
}

boost::asio::awaitable<boost::beast::http::message_generator> actions_projects_tasks_comment_many::post(
    session_data_ptr in_handle
) {
  auto l_sql = g_ctx().get<sqlite_database>();

  SPDLOG_LOGGER_WARN(
      g_logger_ctrl().get_http(), "用户 {}({}) 开始批量创建评论", person_.person_.email_,
      person_.person_.get_full_name()
  );

  std::vector<create_comment_result> l_result{};
  for (auto&& i : in_handle->get_json()) {
    auto l_comm = std::make_shared<comment>(i.get<comment>());
    default_logger_raw()->info("{} 创建评论", person_.person_.email_);
    l_result.emplace_back(co_await create_comment(l_comm, &person_, {}, {}));
  }

  SPDLOG_LOGGER_WARN(
      g_logger_ctrl().get_http(), "用户 {}({}) 完成批量创建评论 数量 {}", person_.person_.email_,
      person_.person_.get_full_name(), l_result.size()
  );

  co_return in_handle->make_msg(nlohmann::json{} = l_result);
}

struct actions_tasks_modify_date_comment_time_arg {
  chrono::system_zoned_time start_date_;
  chrono::system_zoned_time due_date_;
  //  form json
  friend void from_json(const nlohmann::json& j, actions_tasks_modify_date_comment_time_arg& p) {
    j.at("start_date").get_to(p.start_date_);
    j.at("due_date").get_to(p.due_date_);
    if (p.start_date_.get_sys_time() > p.due_date_.get_sys_time()) std::swap(p.start_date_, p.due_date_);
  }
};

boost::asio::awaitable<boost::beast::http::message_generator> actions_tasks_modify_date_comment::post(
    session_data_ptr in_handle
) {
  auto l_sql  = g_ctx().get<sqlite_database>();
  auto l_task = std::make_shared<task>(l_sql.get_by_uuid<task>(id_));

  SPDLOG_LOGGER_WARN(
      g_logger_ctrl().get_http(), "用户 {}({}) 开始修改任务 {} 时间并创建评论", person_.person_.email_,
      person_.person_.get_full_name(), id_
  );

  if (!(l_task->start_date_ && l_task->due_date_))
    co_return in_handle->make_error_code_msg(boost::beast::http::status::not_found, "任务没有设置时间");

  auto l_comment = std::make_shared<comment>();
  auto l_json    = in_handle->get_json();
  l_json.get_to(*l_comment);
  if (!l_comment->text_.empty()) l_comment->text_ = boost::locale::conv::to_utf<char>(l_comment->text_, "UTF-8");
  auto l_arg       = l_json.get<actions_tasks_modify_date_comment_time_arg>();
  l_comment->text_ = fmt::format(
      "原先时间 {:%F} {:%F} 修改到新的的时间 {:%F} {:%F} 原因: {}", *l_task->start_date_, *l_task->due_date_,
      l_arg.start_date_, l_arg.due_date_, l_comment->text_
  );

  l_task->start_date_ = l_arg.start_date_;
  l_task->due_date_   = l_arg.due_date_;
  co_await l_sql.update(l_task);
  auto l_result = co_await create_comment(l_comment, &person_, id_, {}, l_task);
  default_logger_raw()->info("由 {} 创建评论 {}, 修改任务时间", person_.person_.email_, l_comment->uuid_id_);

  SPDLOG_LOGGER_WARN(
      g_logger_ctrl().get_http(), "用户 {}({}) 完成修改任务 {} 时间并创建评论 {}", person_.person_.email_,
      person_.person_.get_full_name(), id_, l_comment->uuid_id_
  );

  co_return in_handle->make_msg(nlohmann::json{} = l_result);
}

boost::asio::awaitable<boost::beast::http::message_generator> data_tasks_comments_ack::post(
    session_data_ptr in_handle
) {
  auto l_sql = g_ctx().get<sqlite_database>();
  std::string l_event_name{};

  SPDLOG_LOGGER_WARN(
      g_logger_ctrl().get_http(), "用户 {}({}) 开始评论 {} 点赞/取消点赞", person_.person_.email_,
      person_.person_.get_full_name(), comment_id_
  );

  using namespace sqlite_orm;
  auto l_task_id = l_sql.impl_->storage_any_.select(&comment::object_id_, where(c(&comment::uuid_id_) == comment_id_));

  if (l_task_id.empty())
    throw_exception(
        http_request_error{boost::beast::http::status::bad_request, fmt::format("未知的评论 id: {}", comment_id_)}
    );
  auto l_prj_id = l_sql.impl_->storage_any_.select(&task::project_id_, where(c(&task::uuid_id_) == l_task_id[0]));
  if (l_prj_id.empty())
    throw_exception(
        http_request_error{boost::beast::http::status::bad_request, fmt::format("未知的task id: {}", l_task_id[0])}
    );
  if (auto l_id = l_sql.impl_->storage_any_.select(
          &comment_acknoledgments::id_, where(
                                            c(&comment_acknoledgments::comment_id_) == comment_id_ &&
                                            c(&comment_acknoledgments::person_id_) == person_.person_.uuid_id_
                                        )
      );
      l_id.empty()) {
    auto l_ack         = std::make_shared<comment_acknoledgments>();
    l_ack->comment_id_ = comment_id_;
    l_ack->person_id_  = person_.person_.uuid_id_;
    co_await l_sql.install(l_ack);
    l_event_name = "comment:acknowledge";
    default_logger_raw()->info("{} 点赞 {}", person_.person_.email_, comment_id_);
  } else {
    l_event_name = "comment:unacknowledge";
    default_logger_raw()->info("{} 取消点赞 {}", person_.person_.email_, comment_id_);
    co_await l_sql.remove<comment_acknoledgments>(l_id[0]);
  }

  socket_io::broadcast(
      l_event_name,
      nlohmann::json{
          {"comment_id", comment_id_}, {"person_id", person_.person_.uuid_id_}, {"project_id", l_prj_id.front()}
      },
      "/events"
  );
  auto l_comment = l_sql.get_by_uuid<comment>(comment_id_);

  SPDLOG_LOGGER_WARN(
      g_logger_ctrl().get_http(), "用户 {}({}) 完成评论 {} 点赞操作 event {}", person_.person_.email_,
      person_.person_.get_full_name(), comment_id_, l_event_name
  );

  co_return in_handle->make_msg(nlohmann::json{} = l_comment);
}

boost::asio::awaitable<boost::beast::http::message_generator> data_comment::get(session_data_ptr in_handle) {
  auto l_sql     = g_ctx().get<sqlite_database>();
  auto l_comment = l_sql.get_by_uuid<comment>(id_);

  nlohmann::json l_json{};
  l_json                     = l_comment;
  l_json["attachment_files"] = l_sql.impl_->storage_any_.get_all<attachment_file>(
      sqlite_orm::where(sqlite_orm::c(&attachment_file::comment_id_) == id_)
  );

  co_return in_handle->make_msg(l_json);
}

boost::asio::awaitable<boost::beast::http::message_generator> task_comment::delete_(session_data_ptr in_handle) {
  auto l_sql  = g_ctx().get<sqlite_database>();
  auto l_task = std::make_shared<task>(l_sql.get_by_uuid<task>(task_id_));
  person_.check_delete_access(l_task->project_id_);
  SPDLOG_LOGGER_WARN(
      g_logger_ctrl().get_http(), "用户 {}({}) 删除 任务 {} 的评论 {}", person_.person_.email_,
      person_.person_.get_full_name(), l_task->uuid_id_, comment_id_
  );
  co_await l_sql.remove<comment>(comment_id_);
  auto l_last_comment = l_sql.get_last_comment(l_task->uuid_id_);
  if (l_last_comment) {
    auto l_task_status         = l_sql.get_by_uuid<task_status>(l_task->task_status_id_);
    l_task->last_comment_date_ = l_last_comment->created_at_;
    l_task->task_status_id_    = l_last_comment->task_status_id_;
    if (l_task_status.is_feedback_request_) l_task->end_date_ = l_last_comment->created_at_;
    if (l_task_status.is_done_) l_task->done_date_ = l_last_comment->created_at_;
    co_await l_sql.update(l_task);
  }
  co_return in_handle->make_msg(nlohmann::json{});
}

boost::asio::awaitable<boost::beast::http::message_generator> data_comment::put(session_data_ptr in_handle) {
  auto l_json    = in_handle->get_json();
  auto l_sql     = g_ctx().get<sqlite_database>();
  auto l_comment = std::make_shared<comment>(l_sql.get_by_uuid<comment>(id_));

  SPDLOG_LOGGER_WARN(
      g_logger_ctrl().get_http(), "用户 {}({}) 开始更新评论 comment_id {}", person_.person_.email_,
      person_.person_.get_full_name(), id_
  );
  l_json.get_to(*l_comment);
  co_await l_sql.update(l_comment);

  SPDLOG_LOGGER_WARN(
      g_logger_ctrl().get_http(), "用户 {}({}) 完成更新评论 comment_id {}", person_.person_.email_,
      person_.person_.get_full_name(), id_
  );
  co_return in_handle->make_msg(nlohmann::json{} = *l_comment);
}

}  // namespace doodle::http