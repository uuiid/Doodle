//
// Created by TD on 24-11-7.
//

#include <doodle_core/metadata/assets.h>
#include <doodle_core/metadata/entity.h>
#include <doodle_core/metadata/entity_type.h>
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
  auto l_ptr  = get_person(in_handle);
  auto& l_sql = g_ctx().get<sqlite_database>();

  auto l_list = l_ptr->person_.role_ == person_role_type::admin ? l_sql.get_project_and_status({})
                                                                : l_sql.get_project_and_status(l_ptr->person_);
  co_return in_handle->make_msg((nlohmann::json{} = l_list).dump());
}

boost::asio::awaitable<boost::beast::http::message_generator> project_get::callback(session_data_ptr in_handle) {
  auto l_ptr        = get_person(in_handle);
  const auto l_uuid = from_uuid_str(in_handle->capture_->get("project_id"));
  auto l_list       = g_ctx().get<sqlite_database>().get_by_uuid<project>(l_uuid);
  nlohmann::json l_j{l_list};
  l_j["project_status_name"] =
      g_ctx().get<sqlite_database>().get_by_uuid<project_status>(l_list.project_status_id_).name_;
  co_return in_handle->make_msg(nlohmann::json{l_list}.dump());
}

boost::asio::awaitable<boost::beast::http::message_generator> project_c_post::callback(session_data_ptr in_handle) {
  auto l_ptr = get_person(in_handle);
  l_ptr->is_manager();

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
  auto l_ptr        = get_person(in_handle);
  auto l_json       = in_handle->get_json();
  auto l_project_id = from_uuid_str(in_handle->capture_->get("project_id"));
  l_ptr->is_project_manager(l_project_id);
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
  auto l_ptr        = get_person(in_handle);
  auto l_json       = in_handle->get_json();
  auto l_project_id = from_uuid_str(in_handle->capture_->get("project_id"));
  l_ptr->is_project_manager(l_project_id);
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
  auto l_ptr        = get_person(in_handle);
  auto l_json       = in_handle->get_json();
  auto l_project_id = from_uuid_str(in_handle->capture_->get("project_id"));
  l_ptr->is_project_manager(l_project_id);
  auto l_prj_asset_type_link            = std::make_shared<project_asset_type_link>();
  l_prj_asset_type_link->asset_type_id_ = l_json["asset_type_id"].get<uuid>();
  l_prj_asset_type_link->project_id_    = l_project_id;
  if (!g_ctx().get<sqlite_database>().get_project_asset_type_link(l_project_id, l_prj_asset_type_link->asset_type_id_))
    co_await g_ctx().get<sqlite_database>().install(l_prj_asset_type_link);
  co_return in_handle->make_msg(
      nlohmann::json{g_ctx().get<sqlite_database>().get_by_uuid<project>(l_project_id)}.dump()
  );
}
boost::asio::awaitable<boost::beast::http::message_generator> actions_create_tasks_post::callback(
    session_data_ptr in_handle
) {
  auto l_ptr          = get_person(in_handle);
  auto l_project_id   = from_uuid_str(in_handle->capture_->get("project_id"));
  auto l_task_type_id = from_uuid_str(in_handle->capture_->get("task_type_id"));
  auto& l_sql         = g_ctx().get<sqlite_database>();
  auto l_task_type    = l_sql.get_by_uuid<task_type>(l_task_type_id);
  std::vector<entity> l_entities{};
  auto l_json = in_handle->get_json();
  if (l_json.is_array())
    for (auto&& l_v : l_json.get<std::vector<uuid>>()) l_entities.emplace_back(l_sql.get_by_uuid<entity>(l_v));
  else
    for (auto&& [l_k, l_v, l_has] : in_handle->url_.params())
      if (l_k == "id" && l_has) l_entities.emplace_back(l_sql.get_by_uuid<entity>(from_uuid_str(l_v)));
  auto l_task_status = l_sql.get_task_status_by_name(std::string{doodle_config::task_status_todo});

  if (l_entities.size() == 1) {
    auto l_task             = std::make_shared<task>();
    l_task->uuid_id_        = core_set::get_set().get_uuid();
    l_task->name_           = "main";
    l_task->project_id_     = l_project_id;
    l_task->task_type_id_   = l_task_type.uuid_id_;
    l_task->entity_id_      = l_entities[0].uuid_id_;
    l_task->task_status_id_ = l_task_status.uuid_id_;
    co_await l_sql.install(l_task);

    nlohmann::json l_json_r{};
    auto&& l_task_json                    = l_json_r.emplace_back(*l_task);
    l_task_json["assignees"]              = nlohmann::json::array();
    l_task_json["task_status_id"]         = l_task_status.uuid_id_;
    l_task_json["task_status_name"]       = l_task_status.name_;
    l_task_json["task_status_short_name"] = l_task_status.short_name_;
    l_task_json["task_status_color"]      = l_task_status.color_;
    l_task_json["task_type_id"]           = l_task_type.uuid_id_;
    l_task_json["task_type_name"]         = l_task_type.name_;
    l_task_json["task_type_color"]        = l_task_type.color_;
    l_task_json["task_type_priority"]     = l_task_type.priority_;

    co_return in_handle->make_msg(l_json_r.dump());
  }
  auto l_tasks = std::make_shared<std::vector<task>>();
  for (auto&& i : l_entities) {
    if (l_sql.is_task_exist(i.uuid_id_, l_task_type.uuid_id_)) continue;
    l_tasks->emplace_back(
        task{
            .uuid_id_        = core_set::get_set().get_uuid(),
            .name_           = "main",
            .project_id_     = i.project_id_,
            .task_type_id_   = l_task_type.uuid_id_,
            .task_status_id_ = l_task_status.uuid_id_,
            .entity_id_      = i.uuid_id_,

        }
    );
  }
  co_await l_sql.install_range(l_tasks);
  nlohmann::json l_json_r{};
  l_json_r = *l_tasks;
  for (auto&& i : l_json_r) {
    i["assignees"]              = nlohmann::json::array();
    i["task_status_id"]         = l_task_status.uuid_id_;
    i["task_status_name"]       = l_task_status.name_;
    i["task_status_short_name"] = l_task_status.short_name_;
    i["task_status_color"]      = l_task_status.color_;
    i["task_type_id"]           = l_task_type.uuid_id_;
    i["task_type_name"]         = l_task_type.name_;
    i["task_type_color"]        = l_task_type.color_;
    i["task_type_priority"]     = l_task_type.priority_;
  }
  co_return in_handle->make_msg(l_json_r.dump());
}
}  // namespace doodle::http

namespace doodle::http::kitsu {
namespace {
boost::asio::awaitable<boost::beast::http::message_generator> put_project(session_data_ptr in_handle) {
  if (in_handle->content_type_ != detail::content_type::application_json)
    co_return in_handle->make_error_code_msg(boost::beast::http::status::bad_request, "错误的请求类型");
  uuid l_uuid        = from_uuid_str(in_handle->capture_->get("id"));

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
