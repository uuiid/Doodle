//
// Created by TD on 24-11-7.
//

#include <doodle_core/metadata/project.h>
#include <doodle_core/sqlite_orm/sqlite_database.h>
#include <doodle_core/sqlite_orm/sqlite_select_data.h>

#include <doodle_lib/core/http/http_function.h>
#include <doodle_lib/core/http/json_body.h>
#include <doodle_lib/core/scan_win_service.h>
#include <doodle_lib/http_method/kitsu/kitsu.h>

#include "kitsu.h"
#include "kitsu_reg_url.h"

namespace doodle::http {
boost::asio::awaitable<boost::beast::http::message_generator> project_all_get::callback(session_data_ptr in_handle) {
  get_person(in_handle);
  auto& l_sql = g_ctx().get<sqlite_database>();

  auto l_list = person_->role_ == person_role_type::admin ? l_sql.get_project_and_status(nullptr)
                                                          : l_sql.get_project_and_status(person_);
  co_return in_handle->make_msg((nlohmann::json{} = l_list).dump());
}

boost::asio::awaitable<boost::beast::http::message_generator> project_get::callback(session_data_ptr in_handle) {
  get_person(in_handle);
  const auto l_uuid = boost::lexical_cast<uuid>(in_handle->capture_->get("project_id"));
  auto l_list       = g_ctx().get<sqlite_database>().get_by_uuid<project>(l_uuid);
  nlohmann::json l_j{l_list};
  l_j["project_status_name"] =
      g_ctx().get<sqlite_database>().get_by_uuid<project_status>(l_list.project_status_id_).name_;
  co_return in_handle->make_msg(nlohmann::json{l_list}.dump());
}

boost::asio::awaitable<boost::beast::http::message_generator> project_c_post::callback(session_data_ptr in_handle) {
  get_person(in_handle);
  if (person_->role_ != person_role_type::admin && person_->role_ != person_role_type::manager)
    throw_exception(http_request_error{boost::beast::http::status::unauthorized, "权限不足"});

  auto l_json = in_handle->get_json();
  auto l_prj  = std::make_shared<project>();
  l_json.get_to(*l_prj);
  l_prj->uuid_id_ = core_set::get_set().get_uuid();
  co_await g_ctx().get<sqlite_database>().install(l_prj);
  co_return in_handle->make_msg(nlohmann::json{*l_prj}.dump());
}
boost::asio::awaitable<boost::beast::http::message_generator> project_settings_task_types_post::callback(
    session_data_ptr in_handle
) {
  get_person(in_handle);
  auto l_json       = in_handle->get_json();
  auto l_project_id = boost::lexical_cast<uuid>(in_handle->capture_->get("project_id"));
  is_project_manager(l_project_id);
  auto l_prj_task_type_link = std::make_shared<project_task_type_link>();
  l_json.get_to(*l_prj_task_type_link);
  l_prj_task_type_link->project_id_ = l_project_id;

  if (auto l_t =
          g_ctx().get<sqlite_database>().get_project_task_type_link(l_project_id, l_prj_task_type_link->task_type_id_);
      !l_t) {
    co_await g_ctx().get<sqlite_database>().install(l_prj_task_type_link);
  }
  co_return in_handle->make_msg(
      nlohmann::json{g_ctx().get<sqlite_database>().get_by_uuid<project>(l_project_id)}.dump()
  );
}
boost::asio::awaitable<boost::beast::http::message_generator> project_settings_task_status_post::callback(
    session_data_ptr in_handle
) {
  get_person(in_handle);
  auto l_json       = in_handle->get_json();
  auto l_project_id = boost::lexical_cast<uuid>(in_handle->capture_->get("project_id"));
  is_project_manager(l_project_id);
  auto l_status_id                        = l_json["task_status_id"].get<uuid>();
  auto l_prj_task_status_link             = std::make_shared<project_task_status_link>();
  l_prj_task_status_link->project_id_     = l_project_id;
  l_prj_task_status_link->task_status_id_ = l_status_id;
  if (!g_ctx().get<sqlite_database>().get_project_task_status_link(l_project_id, l_status_id))
    co_await g_ctx().get<sqlite_database>().install(l_prj_task_status_link);
  co_return in_handle->make_msg(
      nlohmann::json{g_ctx().get<sqlite_database>().get_by_uuid<project>(l_project_id)}.dump()
  );
}
boost::asio::awaitable<boost::beast::http::message_generator> project_settings_asset_types_post::callback(
    session_data_ptr in_handle
) {
  get_person(in_handle);
  auto l_json       = in_handle->get_json();
  auto l_project_id = boost::lexical_cast<uuid>(in_handle->capture_->get("project_id"));
  is_project_manager(l_project_id);
  auto l_prj_asset_type_link            = std::make_shared<project_asset_type_link>();
  l_prj_asset_type_link->asset_type_id_ = l_json["asset_type_id"].get<uuid>();
  l_prj_asset_type_link->project_id_    = l_project_id;
  if (!g_ctx().get<sqlite_database>().get_project_asset_type_link(l_project_id, l_prj_asset_type_link->asset_type_id_))
    co_await g_ctx().get<sqlite_database>().install(l_prj_asset_type_link);
  co_return in_handle->make_msg(
      nlohmann::json{g_ctx().get<sqlite_database>().get_by_uuid<project>(l_project_id)}.dump()
  );
}

}  // namespace doodle::http

namespace doodle::http::kitsu {
namespace {
boost::asio::awaitable<boost::beast::http::message_generator> put_project(session_data_ptr in_handle) {
  if (in_handle->content_type_ != detail::content_type::application_json)
    co_return in_handle->make_error_code_msg(boost::beast::http::status::bad_request, "错误的请求类型");
  uuid l_uuid        = boost::lexical_cast<uuid>(in_handle->capture_->get("id"));

  const auto& l_json = std::get<nlohmann::json>(in_handle->body_);
  auto l_sql         = g_ctx().get<sqlite_database>();
  auto l_prj         = std::make_shared<project_helper::database_t>(
      l_sql.uuid_to_id<project_helper::database_t>(l_uuid) ? l_sql.get_by_uuid<project_helper::database_t>(l_uuid)
                                                           : project_helper::database_t{}
  );
  if (l_prj->uuid_id_.is_nil()) l_prj->uuid_id_ = l_uuid;
  auto l_org_prj = *l_prj;
  l_json.get_to(*l_prj);
  if (l_org_prj != *l_prj) co_await l_sql.install(l_prj);

  detail::http_client_data_base_ptr l_client_data = create_kitsu_proxy(in_handle);
  boost::beast::http::request<boost::beast::http::string_body> l_request{in_handle->req_header_};
  l_request.body() = l_json.dump();
  l_request.prepare_payload();
  auto [l_ec, l_res] = co_await detail::read_and_write<boost::beast::http::string_body>(l_client_data, l_request);
  if (l_ec) {
    co_return in_handle->make_error_code_msg(boost::beast::http::status::internal_server_error, "服务器错误");
  }

  auto l_json_r = nlohmann::json::parse(l_res.body());
  nlohmann::json l_j{};
  l_j = l_sql.get_by_uuid<project_helper::database_t>(l_uuid);
  l_j.update(l_json_r);
  l_res.body() = l_j.dump();
  l_res.prepare_payload();

  co_return std::move(l_res);
}

boost::asio::awaitable<boost::beast::http::message_generator> get_project_all(session_data_ptr in_handle) {
  detail::http_client_data_base_ptr l_client_data = create_kitsu_proxy(in_handle);
  boost::beast::http::request<boost::beast::http::string_body> l_request{in_handle->req_header_};
  auto [l_ec, l_res] = co_await detail::read_and_write<boost::beast::http::string_body>(l_client_data, l_request);
  if (l_ec) {
    co_return in_handle->make_error_code_msg(boost::beast::http::status::internal_server_error, "服务器错误");
  }

  auto l_json = nlohmann::json::parse(l_res.body());
  if (!l_json.is_array()) {
    co_return std::move(l_res);
  }
  for (auto& l_item : l_json) {
    uuid l_id = l_item["id"].get<uuid>();
    nlohmann::json l_j{};
    l_j = g_ctx().get<sqlite_database>().get_by_uuid<project_helper::database_t>(l_id);
    l_j.update(l_item);
    l_item = l_j;
  }

  l_res.body() = l_json.dump();
  l_res.prepare_payload();

  co_return std::move(l_res);
}
boost::asio::awaitable<boost::beast::http::message_generator> post_project(session_data_ptr in_handle) {
  detail::http_client_data_base_ptr l_client_data = create_kitsu_proxy(in_handle);
  boost::beast::http::request<boost::beast::http::string_body> l_request{in_handle->req_header_};
  auto& l_json                   = std::get<nlohmann::json>(in_handle->body_);

  std::shared_ptr<project> l_prj = std::make_shared<project>();
  l_json.get_to(*l_prj);
  // l_prj->uuid_ = core_set::get_set().get_uuid();
  l_request.body()   = l_json.dump();
  auto [l_ec, l_res] = co_await detail::read_and_write<boost::beast::http::string_body>(l_client_data, l_request);
  if (l_ec) {
    co_return in_handle->make_error_code_msg(boost::beast::http::status::internal_server_error, "服务器错误");
  }
  l_prj->uuid_id_ = nlohmann::json::parse(l_res.body()).at("id").get<uuid>();
  co_await g_ctx().get<sqlite_database>().install(l_prj);
  co_return std::move(l_res);
}
}  // namespace
void project_reg(http_route& in_http_route) {
  in_http_route
      .reg(std::make_shared<http_function>(boost::beast::http::verb::put, "api/data/projects/{id}", put_project))
#ifndef NDEBUG
      .reg(std::make_shared<http_function>(boost::beast::http::verb::get, "api/data/projects", get_project_all))
      .reg(std::make_shared<http_function>(boost::beast::http::verb::post, "api/data/projects", post_project))
#endif
      ;
}
}  // namespace doodle::http::kitsu
