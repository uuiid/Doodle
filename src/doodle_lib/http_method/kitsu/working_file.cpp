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

#include <sqlite_orm/sqlite_orm.h>
namespace doodle::http {

namespace {
boost::asio::awaitable<void> scan_working_files() {
  auto l_sql = g_ctx().get<sqlite_database>();

  std::vector<std::int64_t> l_delete_ids{};
  for (auto&& l_f : l_sql.get_all<working_file>()) {
    if (!l_f.path_.empty() && !exists(l_f.path_)) l_delete_ids.emplace_back(l_f.id_);
  }
  if (!l_delete_ids.empty()) co_await l_sql.remove<working_file>(l_delete_ids);

  std::shared_ptr<std::vector<working_file>> l_working_files = std::make_shared<std::vector<working_file>>();
  for (auto&& i : l_sql.impl_->storage_any_.iterate<task>()) {
    try {
      *l_working_files |= ranges::actions::push_back(scan_assets::scan_task(i));
    } catch (const FSys::filesystem_error& l_error) {
      default_logger_raw()->warn("{}", l_error.what());
    }
  }
  if (!l_working_files->empty()) co_await l_sql.install_range(l_working_files);
}
}  // namespace
boost::asio::awaitable<boost::beast::http::message_generator> actions_working_files_scan_all::post(
    session_data_ptr in_handle
) {
  person_.check_admin();
  static std::atomic_bool g_begin_scan{false};
  if (g_begin_scan) co_return in_handle->make_msg_204();

  g_begin_scan = true;
  boost::asio::co_spawn(g_io_context(), scan_working_files(), [](const std::exception_ptr in_eptr) {
    try {
      if (in_eptr) std::rethrow_exception(in_eptr);
    } catch (const std::exception& e) {
      default_logger_raw()->error(e.what());
    };
    g_begin_scan = false;
  });

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
boost::asio::awaitable<boost::beast::http::message_generator> actions_tasks_working_file::get(
    session_data_ptr in_handle
) {
  auto l_sql = g_ctx().get<sqlite_database>();
  using namespace sqlite_orm;
  auto l_task = l_sql.impl_->storage_any_.get_all<working_file>(where(c(&working_file::task_id_) == id_));
  if (l_task.empty())
    in_handle->make_error_code_msg(boost::beast::http::status::not_found, "没有找到对应的working file");
  auto l_it = ranges::find_if(l_task, [&](const working_file& i) { return i.software_type_ == software_enum::maya; });
  if (l_it == l_task.end())
    in_handle->make_error_code_msg(boost::beast::http::status::not_found, "没有找到对应的maya working file");
  if (l_it->path_.empty() || !FSys::exists(l_it->path_))
    in_handle->make_error_code_msg(boost::beast::http::status::not_found, "没有找到对应的maya working file");

  co_return in_handle->make_msg(nlohmann::json{} = *l_it);
}
boost::asio::awaitable<boost::beast::http::message_generator> actions_projects_tasks_working_file_many::post(
    session_data_ptr in_handle
) {
  auto l_sql                                                 = g_ctx().get<sqlite_database>();
  auto l_ids                                                 = in_handle->get_json().get<std::vector<uuid>>();
  std::shared_ptr<std::vector<working_file>> l_working_files = std::make_shared<std::vector<working_file>>();
  std::map<uuid, std::vector<working_file>> l_work_map{};
  for (auto&& i : l_ids) {
    auto l_task                 = l_sql.get_by_uuid<task>(i);
    l_work_map[l_task.uuid_id_] = scan_assets::scan_task(l_task);
    *l_working_files |= ranges::actions::push_back(l_work_map[l_task.uuid_id_]);
  }
  if (!l_working_files->empty()) co_await l_sql.install_range(l_working_files);
  co_return in_handle->make_msg(nlohmann::json{} = l_work_map);
}

}  // namespace doodle::http