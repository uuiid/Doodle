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
  uuid l_uuid{};
  try {
    l_uuid = boost::lexical_cast<uuid>(in_handle->capture_->get("id"));
  } catch (...) {
    co_return in_handle->make_error_code_msg(boost::beast::http::status::bad_request, "错误的请求类型");
  }

  auto& l_json = std::get<nlohmann::json>(in_handle->body_);
  try {
    auto l_prj = std::make_shared<project_helper::database_t>();

    if (auto l_prj_t = g_ctx().get<sqlite_database>().get_by_uuid<project_helper::database_t>(l_uuid);
        !l_prj_t.empty()) {
      *l_prj = l_prj_t.front();
    } else {
      l_prj->uuid_id_ = l_uuid;
    }

    bool l_modified{};
    if (l_json.contains("path") && l_json["path"].is_string())
      l_modified = true, l_prj->path_ = l_json["path"].get<FSys::path>();
    if (l_json.contains("en_str") && l_json["en_str"].is_string())
      l_modified = true, l_prj->en_str_ = l_json["en_str"].get<std::string>();
    if (l_json.contains("auto_upload_path") && l_json["auto_upload_path"].is_string())
      l_modified = true, l_prj->auto_upload_path_ = l_json["auto_upload_path"].get<std::string>();

    if (l_modified)
      if (auto l_e = co_await g_ctx().get<sqlite_database>().install(l_prj); !l_e)
        co_return in_handle->logger_->error("api/data/projects/id {}", l_e.error()),
            in_handle->make_error_code_msg(boost::beast::http::status::internal_server_error, l_e.error());
  } catch (...) {
    in_handle->logger_->error("api/data/projects/id {}", boost::current_exception_diagnostic_information());
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

boost::asio::awaitable<boost::beast::http::message_generator> get_project_all(session_data_ptr in_handle) {
  detail::http_client_data_base_ptr l_client_data = create_kitsu_proxy(in_handle);
  boost::beast::http::request<boost::beast::http::string_body> l_request{in_handle->req_header_};
  auto [l_ec, l_res] = co_await detail::read_and_write<boost::beast::http::string_body>(l_client_data, l_request);
  if (l_ec) {
    co_return in_handle->make_error_code_msg(boost::beast::http::status::internal_server_error, "服务器错误");
  }

  try {
    auto l_json = nlohmann::json::parse(l_res.body());
    if (!l_json.is_array()) {
      co_return std::move(l_res);
    }
    for (auto& l_item : l_json) {
      uuid l_id = l_item["id"].get<uuid>();
      if (auto l_list = g_ctx().get<sqlite_database>().get_by_uuid<project_helper::database_t>(l_id); l_list.empty()) {
        l_item["path"]             = nlohmann::json::value_t::null;
        l_item["en_str"]           = nlohmann::json::value_t::null;
        l_item["auto_upload_path"] = nlohmann::json::value_t::null;
      } else {
        l_item["path"]             = l_list.front().path_.string();
        l_item["en_str"]           = l_list.front().en_str_;
        l_item["auto_upload_path"] = l_list.front().auto_upload_path_;
      }
    }

    l_res.body() = l_json.dump();
    l_res.prepare_payload();
  } catch (...) {
    co_return in_handle->make_error_code_msg(boost::beast::http::status::internal_server_error, "服务器错误");
  }

  co_return std::move(l_res);
}

}  // namespace
void project_reg(http_route& in_http_route) {
  in_http_route
      .reg(std::make_shared<http_function>(boost::beast::http::verb::put, "api/data/projects/{id}", put_project))
      .reg(
          std::make_shared<http_function>(boost::beast::http::verb::get, "api/data/projects", get_project_all)

      );
}
}  // namespace doodle::http::kitsu
