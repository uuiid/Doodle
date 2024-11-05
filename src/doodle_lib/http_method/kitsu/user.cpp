//
// Created by TD on 24-8-21.
//

#include "user.h"

#include <doodle_core/metadata/kitsu/task_type.h>
#include <doodle_core/metadata/user.h>
#include <doodle_core/sqlite_orm/sqlite_database.h>

#include <doodle_lib/core/http/json_body.h>
#include <doodle_lib/http_client/dingding_client.h>
#include <doodle_lib/http_method/kitsu/kitsu.h>
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
  try {
    auto& l_user    = l_json["user"];
    auto l_user_id  = l_user["id"].get<uuid>();

    auto l_users    = g_ctx().get<sqlite_database>().get_by_uuid<user_helper::database_t>(l_user_id);
    auto l_user_ptr = std::make_shared<user_helper::database_t>();
    if (!l_users.empty()) *l_user_ptr = l_users.front();
    std::string l_phone{};
    if (l_user["phone"].is_string()) l_phone = l_user["phone"].get<std::string>();

    if (l_user_ptr->mobile_.value_or(std::string{}) != l_phone ||
        l_user_ptr->power_ != l_user["role"].get<power_enum>()) {
      l_user_ptr->mobile_  = l_phone;
      l_user_ptr->power_   = l_user["role"].get<power_enum>();
      l_user_ptr->uuid_id_ = l_user_id;
      if (auto l_e = co_await g_ctx().get<sqlite_database>().install(l_user_ptr); !l_e)
        co_return in_handle->logger_->error("api/auth/authenticated {}", l_e.error()),
            in_handle->make_error_code_msg(boost::beast::http::status::internal_server_error, l_e.error());
    }
    if (l_user_ptr->dingding_company_id_)
      l_user["dingding_company_id"] = *l_user_ptr->dingding_company_id_;
    else
      l_user["dingding_company_id"] = "";
    l_res.body() = l_json.dump();
    l_res.prepare_payload();

  } catch (...) {
    in_handle->logger_->error("api/auth/authenticated {}", boost::current_exception_diagnostic_information());
  }
  co_return std::move(l_res);
}
boost::asio::awaitable<boost::beast::http::message_generator> user_persons_post(session_data_ptr in_handle) {
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
    auto l_user = std::make_shared<user_helper::database_t>();

    if (auto l_user_t = g_ctx().get<sqlite_database>().get_by_uuid<user_helper::database_t>(l_uuid);
        !l_user_t.empty()) {
      *l_user = l_user_t.front();
    } else {
      l_user->uuid_id_ = l_uuid;
    }
    if (l_json["mobile"].is_string()) l_user->mobile_ = l_json["mobile"].get<std::string>();
    l_user->power_ = l_json["power"].get<power_enum>();
    if (l_json["dingding_company_id"].is_string())
      l_user->dingding_company_id_ = l_json["dingding_company_id"].get<uuid>();

    // l_json.erase("dingding_company_id");

    if (auto l_e = co_await g_ctx().get<sqlite_database>().install(l_user); !l_e)
      co_return in_handle->logger_->error("api/user/persons_post {}", l_e.error()),
          in_handle->make_error_code_msg(boost::beast::http::status::internal_server_error, l_e.error());
  } catch (...) {
    in_handle->logger_->error("api/user/persons_post {}", boost::current_exception_diagnostic_information());
  }
  detail::http_client_data_base_ptr l_client_data = create_kitsu_proxy(in_handle);
  boost::beast::http::request<boost::beast::http::string_body> l_request{in_handle->req_header_};
  l_request.body()   = l_json.dump();
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
  try {
    auto l_json   = nlohmann::json::parse(l_res.body());
    auto l_user_s = g_ctx().get<sqlite_database>().get_all<user_helper::database_t>();
    std::map<uuid, user_helper::database_t*> l_user_maps{};
    for (auto&& l_user : l_user_s) {
      l_user_maps[l_user.uuid_id_] = &l_user;
    }
    for (auto&& l_person : l_json["persons"]) {
      auto l_id = l_person["id"].get<uuid>();
      if (l_user_maps.contains(l_id) && l_user_maps[l_id]->dingding_company_id_) {
        l_person["dingding_company_id"] = *l_user_maps[l_id]->dingding_company_id_;
      } else
        l_person["dingding_company_id"] = "";
    }
    auto l_cs = g_ctx().get<dingding::dingding_company>();
    for (auto& l_v : l_cs.company_info_map_ | std::views::values) {
      l_json["dingding_companys"].emplace_back(l_v);
    }

    l_res.body() = l_json.dump();
    l_res.prepare_payload();
  } catch (...) {
    in_handle->logger_->error("api/data/user/context {}", boost::current_exception_diagnostic_information());
  }
  co_return std::move(l_res);
}

}  // namespace
void user_reg(http_route& in_http_route) {
  in_http_route
      .reg(std::make_shared<http_function>(boost::beast::http::verb::get, "api/auth/authenticated", user_authenticated))
      .reg(std::make_shared<http_function>(boost::beast::http::verb::put, "api/data/persons/{id}", user_persons_post))
      .reg(
          std::make_shared<http_function>(boost::beast::http::verb::get, "api/data/user/context", user_context)

      );
}
}  // namespace doodle::http::kitsu