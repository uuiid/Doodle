//
// Created by TD on 24-8-21.
//

#include "doodle_core/metadata/department.h"
#include "doodle_core/metadata/project_status.h"
#include "doodle_core/metadata/status_automation.h"
#include "doodle_core/metadata/task_status.h"
#include "doodle_core/metadata/task_type.h"
#include <doodle_core/metadata/entity_type.h>
#include <doodle_core/metadata/kitsu/task_type.h>
#include <doodle_core/metadata/studio.h>
#include <doodle_core/metadata/user.h>
#include <doodle_core/sqlite_orm/sqlite_database.h>

#include <doodle_lib/core/http/json_body.h>
#include <doodle_lib/http_client/dingding_client.h>
#include <doodle_lib/http_method/http_jwt_fun.h>
#include <doodle_lib/http_method/kitsu/kitsu.h>
#include <doodle_lib/http_method/kitsu/kitsu_reg_url.h>

#include "kitsu.h"

namespace doodle::http {
boost::asio::awaitable<boost::beast::http::message_generator> user_context_get::callback(session_data_ptr in_handle) {
  get_person(in_handle);
  nlohmann::json l_ret{};
  auto& l_sql             = g_ctx().get<sqlite_database>();
  l_ret["asset_types"]    = l_sql.get_all<asset_type>();
  l_ret["custom_actions"] = nlohmann::json::value_t::array;
  l_ret["departments"]    = l_sql.get_all<department>();
  for (auto& l_v : g_ctx().get<dingding::dingding_company>().company_info_map_ | std::views::values) {
    l_ret["dingding_companys"].emplace_back(l_v);
  }
  l_ret["notification_count"]       = l_sql.get_notification_count(person_->uuid_id_);
  l_ret["persons"]                  = l_sql.get_all<person>();
  l_ret["project_status"]           = l_sql.get_all<project_status>();
  l_ret["preview_background_files"] = nlohmann::json::value_t::array;
  l_ret["projects"]                 = l_sql.get_project_for_user(*person_);
  l_ret["status_automations"]       = l_sql.get_all<status_automation>();
  l_ret["studios"]                  = l_sql.get_all<studio>();
  l_ret["task_status"]              = l_sql.get_all<task_status>();
  l_ret["task_types"]               = l_sql.get_all<task_type>();
  l_ret["user_limit"]               = 9999;
  l_ret["search_filter_groups"]     = nlohmann::json::value_t::object;
  l_ret["search_filters"]           = nlohmann::json::value_t::object;

  co_return in_handle->make_msg(l_ret.dump());
}

boost::asio::awaitable<boost::beast::http::message_generator> person_all_get::callback(session_data_ptr in_handle) {
  get_person(in_handle);
  auto l_p = g_ctx().get<sqlite_database>().get_all<person>();
  // for (auto&& [l_k, l_v, l_has] : in_handle->url_.params()) {
  //   if (l_k == "relations")
  //     for (auto&& i : l_p) i.write_departments_ = true;
  // }
  co_return in_handle->make_msg((nlohmann::json{} = l_p).dump());
}

}  // namespace doodle::http

namespace doodle::http::kitsu {

namespace {

boost::asio::awaitable<boost::beast::http::message_generator> user_authenticated(session_data_ptr in_handle) {
  detail::http_client_data_base_ptr l_client_data = create_kitsu_proxy(in_handle);
  boost::beast::http::request<boost::beast::http::string_body> l_request{in_handle->req_header_};

  auto [l_ec, l_res] = co_await detail::read_and_write<boost::beast::http::string_body>(l_client_data, l_request);
  if (l_ec) {
    co_return in_handle->make_error_code_msg(boost::beast::http::status::internal_server_error, "服务器错误");
  }
  auto l_json = nlohmann::json::parse(l_res.body());

  if (l_res.result() != boost::beast::http::status::ok) co_return std::move(l_res);
  if (!(l_json.contains("user") && l_json["user"].contains("id"))) co_return std::move(l_res);

  auto& l_user    = l_json["user"];
  auto l_user_id  = l_user["id"].get<uuid>();

  auto l_user_ptr = std::make_shared<user_helper::database_t>();
  if (g_ctx().get<sqlite_database>().uuid_to_id<user_helper::database_t>(l_user_id))
    *l_user_ptr = g_ctx().get<sqlite_database>().get_by_uuid<user_helper::database_t>(l_user_id);
  std::string l_phone{};
  if (l_user["phone"].is_string()) l_phone = l_user["phone"].get<std::string>();

  if (l_user_ptr->mobile_ != l_phone || l_user_ptr->power_ != l_user["role"].get<power_enum>()) {
    l_user_ptr->mobile_  = l_phone;
    l_user_ptr->power_   = l_user["role"].get<power_enum>();
    l_user_ptr->uuid_id_ = l_user_id;
    co_await g_ctx().get<sqlite_database>().install(l_user_ptr);
  }
  if (!l_user_ptr->dingding_company_id_.is_nil())
    l_user["dingding_company_id"] = l_user_ptr->dingding_company_id_;
  else
    l_user["dingding_company_id"] = "";
  l_res.body() = l_json.dump();
  l_res.prepare_payload();

  co_return std::move(l_res);
}
boost::asio::awaitable<boost::beast::http::message_generator> user_persons_post(session_data_ptr in_handle) {
  if (in_handle->content_type_ != detail::content_type::application_json)
    co_return in_handle->make_error_code_msg(boost::beast::http::status::bad_request, "错误的请求类型");
  uuid l_uuid  = boost::lexical_cast<uuid>(in_handle->capture_->get("id"));

  auto& l_json = std::get<nlohmann::json>(in_handle->body_);
  if (g_ctx().get<sqlite_database>().uuid_to_id<user_helper::database_t>(l_uuid)) {
    auto l_user = std::make_shared<user_helper::database_t>(
        g_ctx().get<sqlite_database>().get_by_uuid<user_helper::database_t>(l_uuid)
    );
    if (l_json["mobile"].is_string()) l_user->mobile_ = l_json["mobile"].get<std::string>();
    l_user->power_ = l_json["power"].get<power_enum>();
    if (l_json["dingding_company_id"].is_string()) {
      if (auto l_company_id = l_json["dingding_company_id"].get<uuid>();
          !g_ctx().get<dingding::dingding_company>().company_info_map_.contains(l_company_id))
        co_return in_handle->make_error_code_msg(boost::beast::http::status::not_found, "该公司不存在");
      else
        l_user->dingding_company_id_ = l_company_id;
    }

    co_await g_ctx().get<sqlite_database>().install(l_user);
  }
  detail::http_client_data_base_ptr l_client_data = create_kitsu_proxy(in_handle);
  boost::beast::http::request<boost::beast::http::string_body> l_request{in_handle->req_header_};
  l_request.body() = l_json.dump();
  l_request.prepare_payload();
  auto [l_ec, l_res] = co_await detail::read_and_write<boost::beast::http::string_body>(l_client_data, l_request);
  if (l_ec) {
    co_return in_handle->make_error_code_msg(boost::beast::http::status::internal_server_error, "服务器错误");
  }
  co_return std::move(l_res);
}

boost::asio::awaitable<boost::beast::http::message_generator> user_context(session_data_ptr in_handle) {
  detail::http_client_data_base_ptr l_client_data = create_kitsu_proxy(in_handle);
  boost::beast::http::request<boost::beast::http::string_body> l_request{in_handle->req_header_};

  auto [l_ec, l_res] = co_await detail::read_and_write<boost::beast::http::string_body>(l_client_data, l_request);
  if (l_ec) {
    co_return in_handle->make_error_code_msg(boost::beast::http::status::internal_server_error, "服务器错误");
  }
  auto l_json      = nlohmann::json::parse(l_res.body());
  auto& l_database = g_ctx().get<sqlite_database>();
  auto l_user_s    = l_database.get_all<user_helper::database_t>();
  std::map<uuid, user_helper::database_t*> l_user_maps{};
  for (auto&& l_user : l_user_s) {
    l_user_maps[l_user.uuid_id_] = &l_user;
  }
  for (auto&& l_person : l_json["persons"]) {
    auto l_id = l_person["id"].get<uuid>();
    if (l_user_maps.contains(l_id) && !l_user_maps[l_id]->dingding_company_id_.is_nil()) {
      l_person["dingding_company_id"] = l_user_maps[l_id]->dingding_company_id_;
    } else
      l_person["dingding_company_id"] = "";
  }
  auto l_cs = g_ctx().get<dingding::dingding_company>();
  for (auto& l_v : l_cs.company_info_map_ | std::views::values) {
    l_json["dingding_companys"].emplace_back(l_v);
  }

  for (auto&& l_project : l_json["projects"]) {
    auto l_id = l_project["id"].get<uuid>();
    nlohmann::json l_j{};

    l_j = !!l_database.uuid_to_id<project_helper::database_t>(l_id)
              ? l_database.get_by_uuid<project_helper::database_t>(l_id)
              : project_helper::database_t{};
    l_j.update(l_project);
    l_project = l_j;
  }

  l_res.body() = l_json.dump();
  l_res.prepare_payload();

  co_return std::move(l_res);
}

}  // namespace
void user_reg(http_route& in_http_route) {
  in_http_route
      .reg(std::make_shared<http_function>(boost::beast::http::verb::get, "api/auth/authenticated", user_authenticated))
      .reg(std::make_shared<http_function>(boost::beast::http::verb::put, "api/data/persons/{id}", user_persons_post))
      .reg(std::make_shared<http_function>(boost::beast::http::verb::get, "api/data/user/context", user_context));
}
}  // namespace doodle::http::kitsu