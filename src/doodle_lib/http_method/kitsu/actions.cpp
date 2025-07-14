//
// Created by TD on 25-7-14.
//
#include "doodle_core/metadata/organisation.h"
#include "doodle_core/metadata/task.h"
#include <doodle_core/core/bcrypt/bcrypt.h>
#include <doodle_core/metadata/person.h>
#include <doodle_core/sqlite_orm/detail/sqlite_database_impl.h>
#include <doodle_core/sqlite_orm/sqlite_database.h>

#include <doodle_lib/core/http/http_function.h>
#include <doodle_lib/http_method/http_jwt_fun.h>
#include <doodle_lib/http_method/kitsu/kitsu_reg_url.h>

namespace doodle::http {
namespace {
struct actions_tasks_clear_assignation_put_args {
  std::vector<uuid> task_id_;
  uuid person_id_;
  // form json
  friend void from_json(const nlohmann::json& j, actions_tasks_clear_assignation_put_args& p) {
    j.at("task_ids").get_to(p.task_id_);
    if (j.contains("person_id")) j.at("person_id").get_to(p.person_id_);
  }
};
// 获取任务分配的人(连接表)
std::optional<assignees_table> get_task_assignees(uuid in_task_id, uuid in_person_id) {
  auto l_sql = g_ctx().get<sqlite_database>();
  using namespace sqlite_orm;
  auto l_ret = l_sql.impl_->storage_any_.get_all<assignees_table>(
      where(c(&assignees_table::task_id_) == in_task_id && c(&assignees_table::person_id_) == in_person_id)
  );
  return !l_ret.empty() ? std::optional{l_ret.front()} : std::optional<assignees_table>{std::nullopt};
}

}  // namespace
boost::asio::awaitable<boost::beast::http::message_generator> actions_tasks_clear_assignation_put::callback_arg(
    session_data_ptr in_handle
) {
  auto l_ptr  = get_person(in_handle);
  auto l_args = in_handle->get_json().get<actions_tasks_clear_assignation_put_args>();
  auto l_sql  = g_ctx().get<sqlite_database>();
  if (l_args.task_id_.empty()) co_return in_handle->make_msg(nlohmann::json::array());

  auto l_task = l_sql.get_by_uuid<task>(l_args.task_id_.front());
  l_ptr->check_task_assign_access(l_task.project_id_);
  for (auto&& l_i : l_args.task_id_)
    if (auto l_assign = get_task_assignees(l_i, l_args.person_id_); l_assign)
      co_await g_ctx().get<sqlite_database>().remove<assignees_table>(l_assign.value().id_);

  co_return in_handle->make_msg(nlohmann::json{} = l_args.task_id_);
}

}  // namespace doodle::http