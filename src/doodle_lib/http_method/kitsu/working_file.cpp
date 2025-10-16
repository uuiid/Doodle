//
// Created by TD on 25-8-15.
//
#include "doodle_core/doodle_core_fwd.h"
#include "doodle_core/metadata/entity_type.h"
#include "doodle_core/metadata/task.h"
#include <doodle_core/metadata/working_file.h>
#include <doodle_core/sqlite_orm/detail/sqlite_database_impl.h>
#include <doodle_core/sqlite_orm/sqlite_database.h>

#include <doodle_lib/core/scan_assets.h>
#include <doodle_lib/http_method/kitsu.h>
#include <doodle_lib/http_method/kitsu/kitsu_front_end.h>
#include <doodle_lib/http_method/kitsu/kitsu_reg_url.h>

#include <boost/exception/diagnostic_information.hpp>

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
  scan_assets::scan_result l_scan_result{};
  std::size_t l_count{};
  using namespace sqlite_orm;
  for (auto&& i : l_sql.impl_->storage_any_.iterate<task>(
           join<entity>(on(c(&task::entity_id_) == c(&entity::uuid_id_))),
           join<asset_type>(on(c(&entity::entity_type_id_) == c(&asset_type::uuid_id_))),
           where(not_in(&entity::entity_type_id_, l_sql.get_temporal_type_ids()))
       )) {
    try {
      auto l_sc = scan_assets::scan_task(i);
      l_scan_result += *l_sc;
      ++l_count;
    } catch (...) {
      default_logger_raw()->error(" 扫描任务 {} 失败 {}", i.name_, boost::current_exception_diagnostic_information());
    }
  }
  default_logger_raw()->info("扫描完成, {} 个文件", l_count);
  co_await l_scan_result.install_async_sqlite();
  co_return;
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
  auto l_sql  = g_ctx().get<sqlite_database>();
  auto l_task = l_sql.get_by_uuid<task>(id_);

  co_await scan_assets::scan_task_async(l_task);
  co_return in_handle->make_msg(nlohmann::json{} = l_sql.get_working_file_by_task(id_));
}
boost::asio::awaitable<boost::beast::http::message_generator> actions_tasks_working_file::get(
    session_data_ptr in_handle
) {
  auto l_sql = g_ctx().get<sqlite_database>();
  using namespace sqlite_orm;
  auto l_task = l_sql.get_working_file_by_task(id_);
  if (l_task.empty())
    in_handle->make_error_code_msg(boost::beast::http::status::not_found, "没有找到对应的working file");
  co_return in_handle->make_msg(nlohmann::json{} = l_task);
}
boost::asio::awaitable<boost::beast::http::message_generator> actions_projects_tasks_working_file_many::post(
    session_data_ptr in_handle
) {
  auto l_sql = g_ctx().get<sqlite_database>();
  auto l_ids = in_handle->get_json().get<std::vector<uuid>>();
  std::map<uuid, std::vector<working_file>> l_work_map{};
  scan_assets::scan_result l_scan_result{};
  for (auto&& i : l_ids) {
    auto l_task = l_sql.get_by_uuid<task>(i);
    auto l_sc   = scan_assets::scan_task(l_task);
    l_scan_result += *l_sc;
  }
  co_await l_scan_result.install_async_sqlite();

  for (auto&& i : l_ids) {
    l_work_map[i] = l_sql.get_working_file_by_task(i);
  }

  co_return in_handle->make_msg(nlohmann::json{} = l_work_map);
}

}  // namespace doodle::http