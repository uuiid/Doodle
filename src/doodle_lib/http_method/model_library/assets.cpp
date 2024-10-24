//
// Created by TD on 24-10-15.
//

#include "assets.h"

#include <doodle_core/metadata/assets.h>
#include <doodle_core/metadata/assets_file.h>
#include <doodle_core/metadata/user.h>
#include <doodle_core/sqlite_orm/sqlite_database.h>

#include "doodle_lib/core/http/http_function.h"

#include "boost/lexical_cast.hpp"

#include <jwt-cpp/jwt.h>
namespace doodle::http::kitsu {

boost::asio::awaitable<boost::beast::http::message_generator> assets_get(session_data_ptr in_handle) {
  auto l_list = g_ctx().get<sqlite_database>().get_all<assets_file_helper::database_t>();
  nlohmann::json l_json{};
  l_json = l_list;
  co_return in_handle->make_msg(l_json.dump());
}

boost::asio::awaitable<boost::beast::http::message_generator> assets_post(session_data_ptr in_handle) {
  if (in_handle->content_type_ != detail::content_type::application_json)
    co_return in_handle->make_error_code_msg(boost::beast::http::status::bad_request, "错误的请求类型");
  std::shared_ptr<assets_file_helper::database_t> l_ptr = std::make_shared<assets_file_helper::database_t>();
  try {
    *l_ptr = std::get<nlohmann::json>(in_handle->body_).get<assets_file_helper::database_t>();
  } catch (...) {
    co_return in_handle->make_error_code_msg(
        boost::beast::http::status::internal_server_error, boost::current_exception_diagnostic_information()
    );
  }
  if (auto l_list = g_ctx().get<sqlite_database>().uuid_to_id<assets_helper::database_t>(l_ptr->uuid_parent_);
      l_list == 0)
    co_return in_handle->make_error_code_msg(boost::beast::http::status::not_found, "未找到父节点");
  else
    l_ptr->parent_id_ = l_list;

  l_ptr->uuid_id_ = core_set::get_set().get_uuid();
  if (auto l_r = co_await g_ctx().get<sqlite_database>().install<assets_file_helper::database_t>(l_ptr); !l_r)
    co_return in_handle->make_error_code_msg(boost::beast::http::status::internal_server_error, l_r.error());
  co_return in_handle->make_msg((nlohmann::json{} = *l_ptr).dump());
}

boost::asio::awaitable<boost::beast::http::message_generator> assets_post_modify(session_data_ptr in_handle) {
  uuid l_uuid{};

  if (in_handle->content_type_ != detail::content_type::application_json)
    co_return in_handle->make_error_code_msg(boost::beast::http::status::bad_request, "错误的请求类型");
  std::shared_ptr<assets_file_helper::database_t> const l_ptr = std::make_shared<assets_file_helper::database_t>();
  try {
    *l_ptr = std::get<nlohmann::json>(in_handle->body_).get<assets_file_helper::database_t>();
    l_uuid = boost::lexical_cast<uuid>(in_handle->capture_->get("id"));
  } catch (...) {
    co_return in_handle->make_error_code_msg(
        boost::beast::http::status::internal_server_error, boost::current_exception_diagnostic_information()
    );
  }

  if (auto l_list = g_ctx().get<sqlite_database>().uuid_to_id<assets_helper::database_t>(l_ptr->uuid_parent_);
      l_list == 0)
    co_return in_handle->make_error_code_msg(boost::beast::http::status::not_found, "未找到父节点");
  else
    l_ptr->parent_id_ = l_list;

  if (auto l_list = g_ctx().get<sqlite_database>().uuid_to_id<assets_file_helper::database_t>(l_uuid); l_list == 0)
    co_return in_handle->make_error_code_msg(boost::beast::http::status::not_found, "未找到节点");
  else
    l_ptr->id_ = l_list;

  l_ptr->uuid_id_ = l_uuid;
  if (auto l_r = co_await g_ctx().get<sqlite_database>().install<assets_file_helper::database_t>(l_ptr); !l_r)
    co_return in_handle->make_error_code_msg(boost::beast::http::status::internal_server_error, l_r.error());
  co_return in_handle->make_msg((nlohmann::json{} = *l_ptr).dump());
}

boost::asio::awaitable<boost::beast::http::message_generator> assets_delete(session_data_ptr in_handle) {
  auto l_uuid = std::make_shared<uuid>();

  try {
    *l_uuid = boost::lexical_cast<uuid>(in_handle->capture_->get("id"));
  } catch (...) {
    co_return in_handle->make_error_code_msg(boost::beast::http::status::bad_request, "无效的任务id");
  }

  if (auto l_list = g_ctx().get<sqlite_database>().get_by_uuid<assets_file_helper::database_t>(*l_uuid);
      !l_list.empty() && l_list.front().active_)
    co_return in_handle->make_error_code_msg(boost::beast::http::status::bad_request, "节点正在使用中, 无法删除");
  try {
    uuid l_user_uuid{};
    if (in_handle->req_header_.count(boost::beast::http::field::authorization) > 0)
      l_user_uuid =
          boost::lexical_cast<uuid>(jwt::decode(in_handle->req_header_.at(boost::beast::http::field::authorization))
                                        .get_payload_json()["sub"]
                                        .to_str());
    else if (in_handle->req_header_.count(boost::beast::http::field::cookie) > 0) {
      auto l_cookie = in_handle->req_header_.at(boost::beast::http::field::cookie);
      auto l_begin  = l_cookie.find("access_token_cookie=");
      l_user_uuid   = boost::lexical_cast<uuid>(l_cookie.substr(l_begin, l_cookie.find(';', l_begin) - l_begin));
    }
    if (l_user_uuid.is_nil())
      co_return in_handle->make_error_code_msg(boost::beast::http::status::unauthorized, "请先登录");

    if (auto l_list = g_ctx().get<sqlite_database>().get_by_uuid<user_helper::database_t>(l_user_uuid); l_list.empty())
      co_return in_handle->make_error_code_msg(boost::beast::http::status::unauthorized, "无效的token");
    else {
      switch (l_list.front().power_) {
        case power_enum::admin:
        case power_enum::manager:
          break;
        default:
          co_return in_handle->make_error_code_msg(boost::beast::http::status::bad_request, "权限不足");
      }
    }
  } catch (...) {
    co_return in_handle->make_error_code_msg(boost::beast::http::status::bad_request, "无效的token");
  }

  if (auto l_r = co_await g_ctx().get<sqlite_database>().remove<assets_file_helper::database_t>(l_uuid); !l_r)
    co_return in_handle->make_error_code_msg(boost::beast::http::status::internal_server_error, l_r.error());
  co_return in_handle->make_msg("{}");
}

void assets_reg(http_route& in_http_route) {
  in_http_route
      .reg(
          std::make_shared<http_function>(boost::beast::http::verb::get, "api/doodle/model_library/assets", assets_get)
      )
      .reg(
          std::make_shared<http_function>(
              boost::beast::http::verb::post, "api/doodle/model_library/assets", assets_post
          )
      )
      .reg(
          std::make_shared<http_function>(
              boost::beast::http::verb::post, "api/doodle/model_library/assets/{id}", assets_post_modify
          )
      )
      .reg(
          std::make_shared<http_function>(
              boost::beast::http::verb::delete_, "api/doodle/model_library/assets/{id}", assets_delete
          ));
}

}  // namespace doodle::http::kitsu