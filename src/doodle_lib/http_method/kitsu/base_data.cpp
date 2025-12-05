//
// Created by TD on 25-4-8.
//
#include "doodle_core/core/app_base.h"
#include "doodle_core/core/core_set.h"
#include "doodle_core/core/global_function.h"
#include "doodle_core/doodle_core_fwd.h"
#include "doodle_core/metadata/entity_type.h"
#include "doodle_core/metadata/person.h"
#include "doodle_core/sqlite_orm/sqlite_database.h"
#include <doodle_core/metadata/department.h>
#include <doodle_core/metadata/status_automation.h>
#include <doodle_core/metadata/studio.h>
#include <doodle_core/metadata/task_status.h>
#include <doodle_core/metadata/task_type.h>
#include <doodle_core/sqlite_orm/detail/sqlite_database_impl.h>

#include <doodle_lib/core/http/http_listener.h>
#include <doodle_lib/core/socket_io/socket_io_ctx.h>
#include <doodle_lib/http_method/kitsu/kitsu_reg_url.h>

#include <boost/asio/cancellation_type.hpp>
#include <boost/asio/system_timer.hpp>
#include <boost/system/detail/error_code.hpp>

#include "core/http/http_session_data.h"
#include "kitsu_reg_url.h"
#include <chrono>
#include <memory>
#include <spdlog/spdlog.h>
#include <vector>

namespace doodle::http {
boost::asio::awaitable<boost::beast::http::message_generator> departments::get(session_data_ptr in_handle) {
  person_.check_admin();
  auto l_list = g_ctx().get<sqlite_database>().get_all<department>();
  co_return in_handle->make_msg((nlohmann::json{} = l_list).dump());
}
boost::asio::awaitable<boost::beast::http::message_generator> departments_instance::put(session_data_ptr in_handle) {
  person_.check_admin();
  auto l_sql            = g_ctx().get<sqlite_database>();
  auto l_department_ptr = std::make_shared<department>(l_sql.get_by_uuid<department>(id_));
  in_handle->get_json().get_to(*l_department_ptr);
  co_await l_sql.install(l_department_ptr);
  co_return in_handle->make_msg(nlohmann::json{} = *l_department_ptr);
}

boost::asio::awaitable<boost::beast::http::message_generator> studios::get(session_data_ptr in_handle) {
  person_.check_admin();
  auto l_list = g_ctx().get<sqlite_database>().get_all<studio>();
  co_return in_handle->make_msg((nlohmann::json{} = l_list).dump());
}
boost::asio::awaitable<boost::beast::http::message_generator> task_types::get(session_data_ptr in_handle) {
  person_.check_admin();
  auto l_list = g_ctx().get<sqlite_database>().get_all<task_type>();
  co_return in_handle->make_msg((nlohmann::json{} = l_list).dump());
}
boost::asio::awaitable<boost::beast::http::message_generator> custom_actions::get(session_data_ptr in_handle) {
  person_.check_admin();

  co_return in_handle->make_msg(nlohmann::json::array());
}
boost::asio::awaitable<boost::beast::http::message_generator> status_automations::get(session_data_ptr in_handle) {
  person_.check_admin();
  auto l_list = g_ctx().get<sqlite_database>().get_all<status_automation>();
  co_return in_handle->make_msg((nlohmann::json{} = l_list).dump());
}

boost::asio::awaitable<boost::beast::http::message_generator> data_entity_types_instance::put(
    session_data_ptr in_handle
) {
  person_.check_admin();
  auto l_sql            = g_ctx().get<sqlite_database>();
  auto l_asset_type_ptr = std::make_shared<asset_type>(l_sql.get_by_uuid<asset_type>(id_));
  in_handle->get_json().get_to(*l_asset_type_ptr);

  auto l_task_type_asset_type_link_list = std::make_shared<std::vector<task_type_asset_type_link>>();
  for (auto&& l_task_type_id : l_asset_type_ptr->task_types_) {
    l_task_type_asset_type_link_list->emplace_back(
        task_type_asset_type_link{
            .asset_type_id_ = l_asset_type_ptr->uuid_id_,
            .task_type_id_  = l_task_type_id,
        }
    );
  }
  co_await l_sql.remove_task_type_asset_type_link_by_asset_type(l_asset_type_ptr->uuid_id_);
  co_await l_sql.install_range(l_task_type_asset_type_link_list);
  co_await l_sql.install(l_asset_type_ptr);
  co_return in_handle->make_msg(nlohmann::json{} = *l_asset_type_ptr);
}
boost::asio::awaitable<boost::beast::http::message_generator> data_task_status::post(session_data_ptr in_handle) {
  person_.check_admin();
  auto l_sql         = g_ctx().get<sqlite_database>();
  auto l_status      = std::make_shared<task_status>();
  l_status->uuid_id_ = core_set::get_set().get_uuid();
  in_handle->get_json().get_to(*l_status);
  co_await l_sql.install(l_status);
  co_return in_handle->make_msg(nlohmann::json{} = *l_status);
}
boost::asio::awaitable<boost::beast::http::message_generator> data_task_status_instance::put(
    session_data_ptr in_handle
) {
  person_.check_admin();
  auto l_sql    = g_ctx().get<sqlite_database>();
  auto l_status = std::make_shared<task_status>(l_sql.get_by_uuid<task_status>(id_));
  in_handle->get_json().get_to(*l_status);
  co_await l_sql.install(l_status);
  co_return in_handle->make_msg(nlohmann::json{} = *l_status);
}
boost::asio::awaitable<boost::beast::http::message_generator> doodle_backup::post(session_data_ptr in_handle) {
  person_.check_admin();
  FSys::path l_file{
      core_set::get_set().get_cache_root("backup") /
      fmt::format("kitsu_{:%Y_%m_%d_%H_%M_%S}.db", chrono::system_clock::now())
  };
  co_await g_ctx().get<sqlite_database>().backup(l_file);
  co_return in_handle->make_msg(l_file.generic_string());
}

boost::asio::awaitable<boost::beast::http::message_generator> doodle_stop_server::post(session_data_ptr in_handle) {
  person_.check_admin();
  g_ctx().get<detail::http_listener_cancellation_slot>().signal_.emit(boost::asio::cancellation_type::all);
  core_set::get_set().read_only_mode_ = true;
  if (g_ctx().contains<socket_io::sid_ctx>()) {
    g_ctx().get<socket_io::sid_ctx>().on_cancel.emit(boost::asio::cancellation_type::all);
  }

  auto l_timer = std::make_shared<boost::asio::system_timer>(g_io_context());
#ifndef NDEBUG
  l_timer->expires_after(20s);  // 20秒
#else
  l_timer->expires_after(20min);  // 20分钟
#endif
  l_timer->async_wait([l_timer](const boost::system::error_code&) { app_base::Get().stop_app(); });
  SPDLOG_LOGGER_WARN(in_handle->logger_, "用户 {} 停止了服务器", person_.person_.get_full_name());
  co_return in_handle->make_msg(nlohmann::json{} = "server stopping");
}
}  // namespace doodle::http