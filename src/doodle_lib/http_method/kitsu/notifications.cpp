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
struct data_user_notifications_get_result {};
auto get_last_notifications_query(const uuid& in_person_id, const data_user_notifications_get_args& in_args) {
  using namespace sqlite_orm;
  if (in_person_id.is_nil())
    throw_exception(http_request_error{boost::beast::http::status::bad_request, "缺失查询参数"});
  auto l_sql = g_ctx().get<sqlite_database>();
  std::vector<data_user_notifications_get_result> l_ret{};
  constexpr auto author = "author"_alias.for_<person>();
  for (auto&& l_row : l_sql.impl_->storage_any_.select(
           columns(
               object<notification>(true), &project::uuid_id_, &project::name_, &task::uuid_id_, &task::entity_id_,
               &comment::uuid_id_, &comment::task_status_id_, &comment::text_, &comment::replies_,
               &subscription::uuid_id_, author->*&person::uuid_id_
           ),
           from<notification>(),  //
           join<author>(on(c(author->*&person::uuid_id_) == c(&notification::author_id_))),
           join<task>(on(c(&notification::task_id_) == c(&task::uuid_id_))),
           join<project>(on(c(&task::project_id_) == c(&project::uuid_id_))),
           left_outer_join<comment>(on(c(&notification::comment_id_) == c(&comment::uuid_id_))),
           left_outer_join<subscription>(
               on(c(&subscription::task_id_) == c(&task::uuid_id_) && c(&subscription::person_id_) == in_person_id)
           ),

           where(
               c(&notification::person_id_) == in_person_id &&
               (!in_args.after_.has_value() || c(&notification::created_at_) > *in_args.after_) &&
               (!in_args.before_.has_value() || c(&notification::created_at_) < *in_args.before_) &&
               (in_args.task_type_id_.is_nil() || c(&task::task_type_id_) == in_args.task_type_id_) &&
               (in_args.task_status_id_.is_nil() || c(&comment::task_status_id_) == in_args.task_status_id_) &&
               (!in_args.notification_type_.has_value() || c(&notification::type_) == *in_args.notification_type_) &&
               (!in_args.read_.has_value() || c(&notification::read_) == *in_args.read_)
           )
       )) {
  }
  return l_ret;
}

}  // namespace
boost::asio::awaitable<boost::beast::http::message_generator> data_user_notifications_get::callback_arg(
    session_data_ptr in_handle
) {
  in_handle->url_.params() co_return in_handle->make_msg(nlohmann::json::array());
}

}  // namespace doodle::http