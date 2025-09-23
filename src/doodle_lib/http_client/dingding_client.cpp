#include "dingding_client.h"

#include <doodle_core/core/app_base.h>
#include <doodle_core/lib_warp/boost_fmt_beast.h>

namespace doodle::dingding {

bool client::token_is_valid() { return chrono::system_clock::now() > token_time_; }

boost::asio::awaitable<void> client::refresh_token() {
  boost::beast::http::request<boost::beast::http::string_body> req{
      boost::beast::http::verb::post, "/v1.0/oauth2/accessToken", 11
  };
  req.body() = nlohmann::json{{"appKey", app_key}, {"appSecret", app_secret}}.dump();
  req.set(boost::beast::http::field::content_type, "application/json");
  req.set(boost::beast::http::field::host, http_client_core_ptr_->server_ip_);
  boost::beast::http::response<http::basic_json_body> l_res{};
  co_await http_client_core_ptr_->read_and_write(req, l_res, boost::asio::use_awaitable);

  if (l_res.result() != boost::beast::http::status::ok)
    throw_exception(doodle_error{fmt::format("refresh_token error: {}", l_res.result())});

  auto& l_json              = l_res.body();
  access_token_             = l_json["accessToken"].get<std::string>();
  chrono::seconds l_seconds = chrono::seconds(l_json["expireIn"].get<int>());
  token_time_               = chrono::system_clock::now() + l_seconds;
}

void client::access_token(const std::string& in_app_key, const std::string& in_app_secret) {
  app_key    = in_app_key;
  app_secret = in_app_secret;
}

boost::asio::awaitable<std::string> client::get_user_by_mobile(const std::string& in_mobile) {
  if (token_is_valid()) co_await refresh_token();

  std::string l_ret{};
  boost::beast::http::request<boost::beast::http::string_body> req{
      boost::beast::http::verb::post, fmt::format("/topapi/v2/user/getbymobile?access_token={}", access_token_), 11
  };
  req.body() = nlohmann::json{{"mobile", in_mobile}}.dump();
  req.set(boost::beast::http::field::content_type, "application/json");
  req.set(boost::beast::http::field::host, http_client_core_ptr_old_->server_ip_);
  boost::beast::http::response<http::basic_json_body> l_res{};
  co_await http_client_core_ptr_old_->read_and_write(req, l_res, boost::asio::use_awaitable);

  if (l_res.result() != boost::beast::http::status::ok)
    throw_exception(doodle_error{fmt::format("get_user_by_mobile error: {}", l_res.result())});

  auto& l_json = l_res.body();
  l_ret        = l_json["result"]["userid"].get<std::string>();

  co_return l_ret;
}

boost::asio::awaitable<std::vector<client::attendance_update>> client::get_attendance_updatedata(
    const std::string& in_userid, const chrono::local_time_pos& in_work_date

) {
  if (token_is_valid()) co_await refresh_token();

  std::vector<attendance_update> l_ret{};
  boost::beast::http::request<boost::beast::http::string_body> req{
      boost::beast::http::verb::post, fmt::format("/topapi/attendance/getupdatedata?access_token={}", access_token_), 11
  };
  req.body() = nlohmann::json{{"userid", in_userid}, {"work_date", fmt::format("{:%Y-%m-%d}", in_work_date)}}.dump();
  req.set(boost::beast::http::field::content_type, "application/json");
  req.set(boost::beast::http::field::host, http_client_core_ptr_old_->server_ip_);

  boost::beast::http::response<http::basic_json_body> l_res{};
  co_await http_client_core_ptr_old_->read_and_write(req, l_res, boost::asio::use_awaitable);
  if (l_res.result() != boost::beast::http::status::ok)
    throw_exception(doodle_error{fmt::format("get_attendance_updatedata error: {}", l_res.result())});

  auto& l_json = l_res.body();

  l_ret        = l_json["result"]["approve_list"].get<std::vector<attendance_update>>();

  co_return l_ret;
}

}  // namespace doodle::dingding