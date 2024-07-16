#include "user_http.h"

#include "doodle_core/metadata/time_point_wrap.h"
#include "doodle_core/platform/win/register_file_type.h"
#include <doodle_core/metadata/user.h>

#include "doodle_lib/core/http/http_session_data.h"
#include "doodle_lib/core/http/http_websocket_data.h"
#include <doodle_lib/http_client/dingding_client.h>
#include <doodle_lib/http_client/kitsu_client.h>
#include <doodle_lib/http_method/share_fun.h>

namespace doodle::http {
namespace {
boost::asio::awaitable<boost::beast::http::message_generator> user_post(session_data_ptr in_handle) {
  auto l_logger = in_handle->logger_;
  if (in_handle->content_type_ != http::detail::content_type::application_json)
    co_return in_handle->make_error_code_msg(
      boost::beast::http::status::bad_request, boost::system::errc::make_error_code(boost::system::errc::bad_message),
      "不是json请求"
    );

  auto l_json = std::get<nlohmann::json>(in_handle->body_);
  boost::uuids::uuid l_company_id;
  boost::uuids::uuid l_user_id;
  try {
    auto l_company_id_str = l_json.at("company").get<std::string>();
    auto l_user_id_str    = in_handle->capture_->get("user_id");
    l_company_id          = boost::lexical_cast<boost::uuids::uuid>(l_company_id_str);
    l_user_id             = boost::lexical_cast<boost::uuids::uuid>(l_user_id_str);
  } catch (...) {
    co_return in_handle->make_error_code_msg(
      404, boost::current_exception_diagnostic_information()
    );
  }
  if (!g_ctx().get<dingding::dingding_company>().company_info_map_.contains(l_company_id))
    co_return in_handle->make_error_code_msg(
      404, "该公司不存在"
    );

  auto l_dingding_client = g_ctx().get<dingding::dingding_company>().company_info_map_.at(l_company_id).client_ptr;

  auto l_this_exe = co_await boost::asio::this_coro::executor;
  co_await boost::asio::post(boost::asio::bind_executor(g_strand(), boost::asio::use_awaitable));
  auto [l_user_h,l_user] = find_user_handle(*g_reg(), l_user_id);
  co_await boost::asio::post(boost::asio::bind_executor(l_this_exe, boost::asio::use_awaitable));

  if (!l_user_h) {
    l_user.dingding_company_id_ = l_company_id;
    l_user.id_                  = l_user_id;
  }
  auto l_kitsu_client = g_ctx().get<kitsu::kitsu_client_ptr>();

  auto [l_e2, l_m] = co_await l_kitsu_client->get_user(l_user_id);
  if (l_e2)
    co_return in_handle->make_error_code_msg(
      404, l_e2.what()
    );
  l_user.mobile_ = l_m.phone_;
  if (l_user.dingding_id_.empty()) {
    auto [l_e3, l_dingding_user] = co_await l_dingding_client->get_user_by_mobile(l_user.mobile_);
    if (l_e3)
      co_return in_handle->make_error_code_msg(
        404, l_e3.what()
      );

    l_user.dingding_id_ = l_dingding_user;
  }

  co_await boost::asio::post(boost::asio::bind_executor(g_strand(), boost::asio::use_awaitable));

  if (!l_user_h) {
    l_user_h = {*g_reg(), g_reg()->create()};
  }
  l_user_h.emplace_or_replace<user>(l_user);
  co_await boost::asio::post(boost::asio::bind_executor(l_this_exe, boost::asio::use_awaitable));
  nlohmann::json l_json_res{};
  l_json_res["id"]          = fmt::to_string(l_user.id_);
  l_json_res["mobile"]      = l_user.mobile_;
  l_json_res["dingding_id"] = l_user.dingding_id_;
  l_json_res["company"]     = fmt::to_string(l_user.dingding_company_id_);

  boost::beast::http::response<boost::beast::http::string_body> l_res{boost::beast::http::status::ok,
                                                                      in_handle->version_};
  l_res.set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);
  l_res.set(boost::beast::http::field::content_type, "application/json");
  l_res.keep_alive(false);
  l_res.body() = l_json.dump();
  l_res.prepare_payload();
  co_return std::move(l_res);
}


boost::asio::awaitable<boost::beast::http::message_generator> user_get(session_data_ptr in_handle) {
  auto l_logger = in_handle->logger_;

  auto l_user_id_str = in_handle->capture_->get("user_id");
  boost::uuids::uuid l_user_id;
  try {
    l_user_id = boost::lexical_cast<boost::uuids::uuid>(l_user_id_str);
  } catch (const std::exception& e) {
    l_logger->log(log_loc(), level::err, "error: {}", e.what());

    co_return in_handle->make_error_code_msg(
      404, "错误的 uuid 格式"
    );
  } catch (...) {
    co_return in_handle->make_error_code_msg(
      404, boost::current_exception_diagnostic_information());
  }
  auto l_v = std::as_const(*g_reg()).view<const user>();

  boost::beast::http::response<boost::beast::http::string_body> l_res{
      boost::beast::http::status::ok, in_handle->version_,
  };
  l_res.set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);
  l_res.set(boost::beast::http::field::content_type, "application/json");
  l_res.keep_alive(in_handle->keep_alive_);

  auto l_this_exe = co_await boost::asio::this_coro::executor;
  co_await boost::asio::post(boost::asio::bind_executor(g_strand(), boost::asio::use_awaitable));
  auto [l_h,l_user] = find_user_handle(*g_reg(), l_user_id);
  co_await boost::asio::post(boost::asio::bind_executor(l_this_exe, boost::asio::use_awaitable));

  if (l_h) {
    nlohmann::json l_json;
    l_json["id"]          = fmt::to_string(l_user.id_);
    l_json["mobile"]      = l_user.mobile_;
    l_json["dingding_id"] = l_user.dingding_id_;
    l_json["company"]     = fmt::to_string(l_user.dingding_company_id_);
    l_res.body()          = l_json.dump();
    l_res.prepare_payload();
    co_return std::move(l_res);
  }
  l_res.result(boost::beast::http::status::not_found);
  l_res.body() = nlohmann::json{{"code", boost::beast::http::status::not_found}, {"message", "未找到用户"}}.dump();
  l_res.prepare_payload();
  co_return std::move(l_res);
}
}

void reg_user_http(http_route& in_route) {
  in_route
      .reg(std::make_shared<http_function>(
        boost::beast::http::verb::get, "api/doodle/user/{user_id}", user_get
      ))
      .reg(std::make_shared<http_function>(
        boost::beast::http::verb::post, "api/doodle/user/{user_id}",
        user_post
      ));
}
} // namespace doodle::http