//
// Created by TD on 24-11-7.
//

#include "project.h"

#include <doodle_core/metadata/project.h>
#include <doodle_core/sqlite_orm/sqlite_database.h>

#include <doodle_lib/core/http/http_function.h>
#include <doodle_lib/core/http/json_body.h>
#include <doodle_lib/core/scan_win_service.h>
#include <doodle_lib/http_method/kitsu/kitsu.h>
namespace doodle::http::kitsu {
namespace {
boost::asio::awaitable<boost::beast::http::message_generator> put_project(session_data_ptr in_handle) {
  if (in_handle->content_type_ != detail::content_type::application_json)
    co_return in_handle->make_error_code_msg(boost::beast::http::status::bad_request, "错误的请求类型");
  uuid l_uuid        = boost::lexical_cast<uuid>(in_handle->capture_->get("id"));

  const auto& l_json = std::get<nlohmann::json>(in_handle->body_);

  auto l_prj         = std::make_shared<project_helper::database_t>(
      g_ctx().get<sqlite_database>().get_by_uuid<project_helper::database_t>(l_uuid)
  );

  auto l_org_prj = *l_prj;
  l_json.get_to(*l_prj);
  if (l_org_prj != *l_prj) co_await g_ctx().get<sqlite_database>().install(l_prj);

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
  l_j = g_ctx().get<sqlite_database>().get_by_uuid<project_helper::database_t>(l_uuid);
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
