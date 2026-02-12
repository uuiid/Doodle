//
// Created by TD on 25-7-14.
//
#include <doodle_core/metadata/entity.h>
#include <doodle_core/sqlite_orm/detail/sqlite_database_impl.h>
#include <doodle_core/sqlite_orm/sqlite_database.h>
#include <doodle_core/sqlite_orm/sqlite_select_data.h>

#include <doodle_lib/http_method/kitsu/kitsu_reg_url.h>

namespace doodle::http {

namespace {

struct data_user_notifications_get_args {
  std::optional<chrono::system_zoned_time> after_;
  std::optional<chrono::system_zoned_time> before_;
  uuid task_type_id_;
  uuid task_status_id_;
  std::optional<notification_type> notification_type_;
  std::optional<bool> read_;

  void get_query_params(const boost::urls::params_ref& in_params) {
    for (auto&& [key, value, has] : in_params) {
      if (key == "after" && has) after_ = from_chrono_time_zone_str(value);
      if (key == "before" && has) before_ = from_chrono_time_zone_str(value);
      if (key == "task_type_id" && has) task_type_id_ = from_uuid_str(value);
      if (key == "task_status_id" && has) task_status_id_ = from_uuid_str(value);
      if (key == "notification_type" && has) notification_type_ = magic_enum::enum_cast<notification_type>(value);
      if (key == "read" && has) read_ = value == "true";
    }
  }
};
struct data_user_notifications_get_result {
  data_user_notifications_get_result() = default;
  data_user_notifications_get_result(
      const notification& in_notification, const entity& in_entity, const comment& in_comment, const uuid& project_id,
      const std::string& project_name, const uuid& task_type_id, const uuid& subscription_id,
      const uuid& in_preview_file_id
  )
      : id_(in_notification.uuid_id_),
        notification_type_(in_notification.type_),
        author_id_(in_notification.author_id_),
        comment_id_(in_comment.uuid_id_),
        task_id_(in_notification.task_id_),
        task_type_id_(task_type_id),
        task_status_id_(in_comment.task_status_id_),
        mentions_(in_comment.mentions_),
        department_mentions_(),
        reply_mentions_(),
        reply_department_mentions_(),
        preview_file_id_(in_preview_file_id),
        project_id_(project_id),
        project_name_(project_name),
        comment_text_(in_comment.text_),
        reply_text_(),
        created_at_(in_notification.created_at_),
        read_(in_notification.read_),
        change_(in_notification.change_),
        full_entity_name_(),
        episode_id_(),
        entity_preview_file_id_(in_entity.preview_file_id_),
        subscription_id_(subscription_id) {
    auto&& [l_full_name, l_epsode_id] = in_entity.get_full_name();
    full_entity_name_                 = l_full_name;
    episode_id_                       = l_epsode_id;
    if (in_notification.type_ == notification_type::reply ||
        in_notification.type_ == notification_type::reply_mention) {
      if (auto l_it = ranges::find_if(
              in_comment.replies_,
              [&](const nlohmann::json& in_reply) { return in_reply["id"].get<uuid>() == in_notification.reply_id_; }
          );
          l_it != in_comment.replies_.end()) {
        auto&& l_reply             = *l_it;
        reply_text_                = l_reply["text"].get<std::string>();
        reply_department_mentions_ = l_reply["department_mentions"].get<std::vector<uuid>>();
        reply_mentions_            = l_reply["mentions"].get<std::vector<uuid>>();
      }
    }
  }
  decltype(notification::uuid_id_) id_;
  decltype(notification::type_) notification_type_;
  decltype(notification::author_id_) author_id_;
  decltype(notification::comment_id_) comment_id_;
  decltype(task::uuid_id_) task_id_;
  decltype(task_type::uuid_id_) task_type_id_;
  decltype(task_status::uuid_id_) task_status_id_;
  decltype(comment::mentions_) mentions_;
  decltype(comment::department_mentions_) department_mentions_;
  std::vector<uuid> reply_mentions_;
  std::vector<uuid> reply_department_mentions_;
  decltype(comment::preview_file_id_) preview_file_id_;
  decltype(project::uuid_id_) project_id_;
  decltype(project::name_) project_name_;
  decltype(comment::text_) comment_text_;
  decltype(comment::replies_) reply_text_;
  decltype(notification::created_at_) created_at_;
  decltype(notification::read_) read_;
  decltype(notification::change_) change_;
  decltype(entity::name_) full_entity_name_;
  decltype(entity::uuid_id_) episode_id_;
  decltype(entity::preview_file_id_) entity_preview_file_id_;
  decltype(subscription::uuid_id_) subscription_id_;

  // to json
  friend void to_json(nlohmann::json& j, const data_user_notifications_get_result& p) {
    j["id"]                        = p.id_;
    j["notification_type"]         = p.notification_type_;
    j["author_id"]                 = p.author_id_;
    j["comment_id"]                = p.comment_id_;
    j["task_id"]                   = p.task_id_;
    j["task_type_id"]              = p.task_type_id_;
    j["task_status_id"]            = p.task_status_id_;
    j["mentions"]                  = p.mentions_;
    j["department_mentions"]       = p.department_mentions_;
    j["reply_mentions"]            = p.reply_mentions_;
    j["reply_department_mentions"] = p.reply_department_mentions_;
    j["preview_file_id"]           = p.preview_file_id_;
    j["project_id"]                = p.project_id_;
    j["project_name"]              = p.project_name_;
    j["comment_text"]              = p.comment_text_;
    j["reply_text"]                = p.reply_text_;
    j["created_at"]                = p.created_at_;
    j["read"]                      = p.read_;
    j["change"]                    = p.change_;
    j["full_entity_name"]          = p.full_entity_name_;
    j["episode_id"]                = p.episode_id_;
    j["entity_preview_file_id"]    = p.entity_preview_file_id_;
    j["subscription_id"]           = p.subscription_id_;
  }
};
auto get_last_notifications_query(const uuid& in_person_id, const data_user_notifications_get_args& in_args) {
  using namespace sqlite_orm;
  if (in_person_id.is_nil())
    throw_exception(http_request_error{boost::beast::http::status::bad_request, "缺失查询参数"});
  auto l_sql = g_ctx().get<sqlite_database>();
  std::vector<data_user_notifications_get_result> l_ret{};
  // constexpr auto author = "author"_alias.for_<person>();
  for (auto&& [

           l_notification, l_entity, l_comment, project_id, project_name, task_type_id, subscription_id

  ] :
       l_sql.impl_->storage_any_.select(
           columns(
               object<notification>(true), object<entity>(true), object<comment>(true), &project::uuid_id_,
               &project::name_, &task::task_type_id_, &subscription::uuid_id_
           ),
           from<notification>(),  //
           join<task>(on(c(&notification::task_id_) == c(&task::uuid_id_))),
           join<project>(on(c(&task::project_id_) == c(&project::uuid_id_))),
           left_outer_join<entity>(on(c(&task::entity_id_) == c(&entity::uuid_id_))),
           left_outer_join<comment>(on(c(&notification::comment_id_) == c(&comment::uuid_id_))),
           left_outer_join<subscription>(
               on(c(&subscription::task_id_) == c(&task::uuid_id_) && c(&subscription::person_id_) == in_person_id)
           ),

           where(
               c(&notification::person_id_) == in_person_id &&
               (!in_args.after_.has_value() ||
                c(&notification::created_at_) > in_args.after_.value_or(chrono::system_zoned_time{})) &&
               (!in_args.before_.has_value() ||
                c(&notification::created_at_) < in_args.before_.value_or(chrono::system_zoned_time{})) &&
               (in_args.task_type_id_.is_nil() || c(&task::task_type_id_) == in_args.task_type_id_) &&
               (in_args.task_status_id_.is_nil() || c(&comment::task_status_id_) == in_args.task_status_id_) &&
               (!in_args.notification_type_.has_value() ||
                c(&notification::type_) == in_args.notification_type_.value_or(notification_type::comment)) &&
               (!in_args.read_.has_value() || c(&notification::read_) == in_args.read_.value_or(true))
           )
       )) {
    auto l_preview_file_id = l_sql.get_preview_file_for_comment(l_comment.uuid_id_).value_or(preview_file{}).uuid_id_;
    l_comment.mentions_    = l_sql.impl_->storage_any_.select(
        &comment_mentions::person_id_, where(c(&comment_mentions::comment_id_) == l_comment.uuid_id_)
    );
    l_comment.department_mentions_ = l_sql.impl_->storage_any_.select(
        &comment_department_mentions::department_id_,
        where(c(&comment_department_mentions::comment_id_) == l_comment.uuid_id_)
    );
    l_ret.emplace_back(
        l_notification, l_entity, l_comment, project_id, project_name, task_type_id, subscription_id, l_preview_file_id

    );
  }
  return l_ret;
}

}  // namespace
boost::asio::awaitable<boost::beast::http::message_generator> data_user_notifications::get(session_data_ptr in_handle) {
  data_user_notifications_get_args l_args{};
  l_args.get_query_params(in_handle->url_.params());
  auto l_ret = get_last_notifications_query(person_.person_.uuid_id_, l_args);
  co_return in_handle->make_msg(nlohmann::json{} = l_ret);
}
boost::asio::awaitable<boost::beast::http::message_generator> data_user_notification::put(session_data_ptr in_handle) {
  auto l_sql   = g_ctx().get<sqlite_database>();
  auto l_not   = std::make_shared<notification>(l_sql.get_by_uuid<notification>(id_));
  const bool l_read = in_handle->get_json().value<bool>("read", false);

  SPDLOG_LOGGER_WARN(
      g_logger_ctrl().get_http(), "用户 {}({}) 开始更新通知 notification_id {} read {}", person_.person_.email_,
      person_.person_.get_full_name(), id_, l_read
  );
  l_not->read_ = l_read;
  co_await l_sql.update(l_not);

  SPDLOG_LOGGER_WARN(
      g_logger_ctrl().get_http(), "用户 {}({}) 完成更新通知 notification_id {} read {}", person_.person_.email_,
      person_.person_.get_full_name(), id_, l_not->read_
  );
  co_return in_handle->make_msg(nlohmann::json{} = *l_not);
}
boost::asio::awaitable<boost::beast::http::message_generator> actions_user_notifications_mark_all_as_read::post(
    session_data_ptr in_handle
) {
  auto l_sql = g_ctx().get<sqlite_database>();

  SPDLOG_LOGGER_WARN(
      g_logger_ctrl().get_http(), "用户 {}({}) 开始将所有通知标记已读", person_.person_.email_,
      person_.person_.get_full_name()
  );

  co_await l_sql.mark_all_notifications_as_read(person_.person_.uuid_id_);

  SPDLOG_LOGGER_WARN(
      g_logger_ctrl().get_http(), "用户 {}({}) 完成将所有通知标记已读", person_.person_.email_,
      person_.person_.get_full_name()
  );

  co_return in_handle->make_msg(nlohmann::json{{"success", true}});
}

}  // namespace doodle::http