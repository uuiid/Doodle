//
// Created by TD on 25-8-15.
//
#include <doodle_core/metadata/working_file.h>
#include <doodle_core/sqlite_orm/detail/sqlite_database_impl.h>
#include <doodle_core/sqlite_orm/sqlite_database.h>

#include <doodle_lib/core/scan_assets.h>
#include <doodle_lib/http_method/kitsu/kitsu.h>
#include <doodle_lib/http_method/kitsu/kitsu_front_end.h>
#include <doodle_lib/http_method/kitsu/kitsu_reg_url.h>
namespace doodle::http {
boost::asio::awaitable<boost::beast::http::message_generator> actions_working_files_scan_all::post(
    session_data_ptr in_handle
) {
  person_.check_admin();
  auto l_sql = g_ctx().get<sqlite_database>();

  std::vector<std::int64_t> l_delete_ids{};
  for (auto&& l_f : l_sql.get_all<working_file>()) {
    if (!l_f.path_.empty() && !exists(l_f.path_)) l_delete_ids.emplace_back(l_f.id_);
  }
  if (!l_delete_ids.empty()) co_await l_sql.remove<working_file>(l_delete_ids);

  std::shared_ptr<std::vector<working_file>> l_working_files = std::make_shared<std::vector<working_file>>();
  for (auto&& i : l_sql.impl_->storage_any_.iterate<task>()) {
    *l_working_files |= ranges::actions::push_back(scan_assets::scan_task(i));
  }
  if (!l_working_files->empty()) co_await l_sql.install_range(l_working_files);
  co_return in_handle->make_msg_204();
}
boost::asio::awaitable<boost::beast::http::message_generator> actions_tasks_working_file::post(
    session_data_ptr in_handle
) {
  auto l_sql   = g_ctx().get<sqlite_database>();
  auto l_task  = l_sql.get_by_uuid<task>(id_);

  auto l_files = co_await scan_assets::scan_task_async(l_task);
  co_return in_handle->make_msg(nlohmann::json{} = *l_files);
}

boost::asio::awaitable<boost::beast::http::message_generator> actions_projects_tasks_working_file_many::post(
    session_data_ptr in_handle
) {
  auto l_sql                                                 = g_ctx().get<sqlite_database>();
  auto l_ids                                                 = in_handle->get_json().get<std::vector<uuid>>();
  std::shared_ptr<std::vector<working_file>> l_working_files = std::make_shared<std::vector<working_file>>();
  for (auto&& i : l_ids) {
    auto l_task = l_sql.get_by_uuid<task>(i);
    *l_working_files |= ranges::actions::push_back(scan_assets::scan_task(l_task));
  }
  if (!l_working_files->empty()) co_await l_sql.install_range(l_working_files);
  co_return in_handle->make_msg(nlohmann::json{} = *l_working_files);
}

}  // namespace doodle::http