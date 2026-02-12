//
// Created by TD on 25-4-8.
//
#include "doodle_core/core/app_base.h"
#include "doodle_core/core/core_set.h"
#include "doodle_core/core/global_function.h"
#include "doodle_core/doodle_core_fwd.h"
#include "doodle_core/exception/exception.h"
#include "doodle_core/metadata/entity_type.h"
#include "doodle_core/metadata/person.h"
#include "doodle_core/sqlite_orm/sqlite_database.h"
#include <doodle_core/metadata/department.h>
#include <doodle_core/metadata/status_automation.h>
#include <doodle_core/metadata/studio.h>
#include <doodle_core/metadata/task_status.h>
#include <doodle_core/metadata/task_type.h>

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
  co_await l_sql.update(l_department_ptr);
  co_return in_handle->make_msg(nlohmann::json{} = *l_department_ptr);
}

boost::asio::awaitable<boost::beast::http::message_generator> studios::get(session_data_ptr in_handle) {
  person_.check_admin();
  auto l_list = g_ctx().get<sqlite_database>().get_all<studio>();
  co_return in_handle->make_msg((nlohmann::json{} = l_list).dump());
}
DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(studios, post) {
  person_.check_admin();
  auto l_sql    = g_ctx().get<sqlite_database>();
  auto l_studio = std::make_shared<studio>();
  in_handle->get_json().get_to(*l_studio);
  DOODLE_CHICK(!l_studio->name_.empty(), "工作室名称不可为空");
  SPDLOG_LOGGER_WARN(
      g_logger_ctrl().get_http(), "用户 {}({}) 开始创建工作室 name {}", person_.person_.email_,
      person_.person_.get_full_name(), l_studio->name_
  );
  co_await l_sql.install(l_studio);
  SPDLOG_LOGGER_WARN(
      g_logger_ctrl().get_http(), "用户 {}({}) 完成创建工作室 studio_id {} name {}", person_.person_.email_,
      person_.person_.get_full_name(), l_studio->uuid_id_, l_studio->name_
  );
  co_return in_handle->make_msg(nlohmann::json{} = *l_studio);
}
DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(studios_instance, delete_) {
  person_.check_admin();
  auto l_sql    = g_ctx().get<sqlite_database>();
  auto l_studio = l_sql.get_by_uuid<studio>(id_);
  SPDLOG_LOGGER_WARN(
      g_logger_ctrl().get_http(), "用户 {}({}) 删除 工作室 {}", person_.person_.email_,
      person_.person_.get_full_name(), l_studio.name_
  );
  co_await l_sql.remove<studio>(l_studio.uuid_id_);
  co_return in_handle->make_msg_204();
}
DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(studios_instance, put) {
  person_.check_admin();
  auto l_sql        = g_ctx().get<sqlite_database>();
  auto l_studio_ptr = std::make_shared<studio>(l_sql.get_by_uuid<studio>(id_));
  in_handle->get_json().get_to(*l_studio_ptr);
  DOODLE_CHICK(!l_studio_ptr->name_.empty(), "工作室名称不可为空");
  co_await l_sql.update(l_studio_ptr);
  co_return in_handle->make_msg(nlohmann::json{} = *l_studio_ptr);
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
DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(status_automations, post) {
  person_.check_admin();
  auto l_sql               = g_ctx().get<sqlite_database>();
  auto l_status_automation = std::make_shared<status_automation>();
  in_handle->get_json().get_to(*l_status_automation);
  SPDLOG_LOGGER_WARN(
      g_logger_ctrl().get_http(), "用户 {}({}) 开始创建状态自动化", person_.person_.email_,
      person_.person_.get_full_name()
  );
  co_await l_sql.install(l_status_automation);
  SPDLOG_LOGGER_WARN(
      g_logger_ctrl().get_http(), "用户 {}({}) 完成创建状态自动化 status_automation_id {}", person_.person_.email_,
      person_.person_.get_full_name(), l_status_automation->uuid_id_
  );
  co_return in_handle->make_msg(nlohmann::json{} = *l_status_automation);
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
  co_await l_sql.update(l_asset_type_ptr);
  co_return in_handle->make_msg(nlohmann::json{} = *l_asset_type_ptr);
}
boost::asio::awaitable<boost::beast::http::message_generator> data_task_status::post(session_data_ptr in_handle) {
  person_.check_admin();
  auto l_sql    = g_ctx().get<sqlite_database>();
  auto l_status = std::make_shared<task_status>();
  in_handle->get_json().get_to(*l_status);
  SPDLOG_LOGGER_WARN(
      g_logger_ctrl().get_http(), "用户 {}({}) 开始创建任务状态 name {}", person_.person_.email_,
      person_.person_.get_full_name(), l_status->name_
  );
  co_await l_sql.install(l_status);
  SPDLOG_LOGGER_WARN(
      g_logger_ctrl().get_http(), "用户 {}({}) 完成创建任务状态 task_status_id {} name {}", person_.person_.email_,
      person_.person_.get_full_name(), l_status->uuid_id_, l_status->name_
  );
  co_return in_handle->make_msg(nlohmann::json{} = *l_status);
}
boost::asio::awaitable<boost::beast::http::message_generator> data_task_status_instance::put(
    session_data_ptr in_handle
) {
  person_.check_admin();
  auto l_sql    = g_ctx().get<sqlite_database>();
  auto l_status = std::make_shared<task_status>(l_sql.get_by_uuid<task_status>(id_));
  in_handle->get_json().get_to(*l_status);
  co_await l_sql.update(l_status);
  co_return in_handle->make_msg(nlohmann::json{} = *l_status);
}
boost::asio::awaitable<boost::beast::http::message_generator> doodle_backup::post(session_data_ptr in_handle) {
  person_.check_admin();
  FSys::path l_file{
      core_set::get_set().get_cache_root("backup") /
      fmt::format("kitsu_{:%Y_%m_%d_%H_%M_%S}.db", chrono::system_clock::now())
  };
  SPDLOG_LOGGER_WARN(
      g_logger_ctrl().get_http(), "用户 {}({}) 开始备份数据库 filename {}", person_.person_.email_,
      person_.person_.get_full_name(), l_file.filename().generic_string()
  );
  co_await g_ctx().get<sqlite_database>().backup(l_file);
  SPDLOG_LOGGER_WARN(
      g_logger_ctrl().get_http(), "用户 {}({}) 完成备份数据库 filename {} size {}", person_.person_.email_,
      person_.person_.get_full_name(), l_file.filename().generic_string(),
      FSys::exists(l_file) ? FSys::file_size(l_file) : 0
  );
  co_return in_handle->make_msg(l_file.generic_string());
}

boost::asio::awaitable<boost::beast::http::message_generator> doodle_stop_server::post(session_data_ptr in_handle) {
  person_.check_admin();
  SPDLOG_LOGGER_WARN(
      g_logger_ctrl().get_http(), "用户 {}({}) 开始停止服务器", person_.person_.email_, person_.person_.get_full_name()
  );
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
  SPDLOG_LOGGER_WARN(
      g_logger_ctrl().get_http(), "用户 {}({}) 已触发停止服务器流程", person_.person_.email_,
      person_.person_.get_full_name()
  );
  co_return in_handle->make_msg(nlohmann::json{} = "server stopping");
}
}  // namespace doodle::http