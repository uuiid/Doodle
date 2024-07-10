#include "dingding_client.h"

#include <doodle_core/core/app_base.h>
#include <doodle_core/lib_warp/boost_fmt_beast.h>

namespace doodle::dingding {
boost::asio::awaitable<void> client::begin_refresh_token() {
  do {
    if ((co_await boost::asio::this_coro::cancellation_state).cancelled() != boost::asio::cancellation_type::none) {
      co_return;
    }

    boost::beast::http::request<boost::beast::http::string_body> req{
        boost::beast::http::verb::post, "/v1.0/oauth2/accessToken", 11
    };
    req.body() = nlohmann::json{{"appKey", app_key}, {"appSecret", app_secret}}.dump();
    req.set(boost::beast::http::field::content_type, "application/json");
    req.set(boost::beast::http::field::host, http_client_core_ptr_->server_ip_);
    auto [l_e, l_res] = co_await http::detail::read_and_write<http::basic_json_body>(
        http_client_core_ptr_, header_operator_req(std::move(req))
    );
    if (l_e) {
      http_client_core_ptr_->logger_->log(log_loc(), level::warn, "read_and_write error: {}", l_e);
      co_return;
    }
    if (l_res.result() != boost::beast::http::status::ok) {
      http_client_core_ptr_->logger_->log(log_loc(), level::warn, "read_and_write error: {}", l_res.result());
      co_return;
    }

    auto& l_json              = l_res.body();
    access_token_             = l_json["accessToken"].get<std::string>();
    chrono::seconds l_seconds = chrono::seconds(l_json["expireIn"].get<int>());
    if (!auto_expire_) co_return;

    timer_ptr_->expires_after(l_seconds);
    auto [l_ex] = co_await timer_ptr_->async_wait();
    if (l_ex) {
      http_client_core_ptr_->logger_->log(log_loc(), level::warn, "timer_ptr_ error: {}", l_ex);
      co_return;
    }
  } while (auto_expire_);
}
void client::access_token(const std::string& in_app_key, const std::string& in_app_secret, bool in_auto_expire) {
  app_key      = in_app_key;
  app_secret   = in_app_secret;
  auto_expire_ = in_auto_expire;

  boost::asio::co_spawn(
      http_client_core_ptr_->get_executor(), begin_refresh_token(),
      boost::asio::bind_cancellation_slot(app_base::Get().on_cancel.slot(), boost::asio::detached)
  );
}

boost::asio::awaitable<void> client::async_access_token(
    const std::string& in_app_key, const std::string& in_app_secret, bool in_auto_expire
) {
  app_key      = in_app_key;
  app_secret   = in_app_secret;
  auto_expire_ = in_auto_expire;
  co_return co_await begin_refresh_token();
}

boost::asio::awaitable<std::tuple<boost::system::error_code, std::string>> client::get_user_by_mobile(
    const std::string& in_mobile
) {
  std::string l_ret{};
  boost::beast::http::request<boost::beast::http::string_body> req{
      boost::beast::http::verb::post, fmt::format("/topapi/v2/user/getbymobile?access_token={}", access_token_), 11
  };
  req.body() = nlohmann::json{{"mobile", in_mobile}}.dump();
  req.set(boost::beast::http::field::content_type, "application/json");
  req.set(boost::beast::http::field::host, http_client_core_ptr_old_->server_ip_);
  auto [l_e, l_res] = co_await http::detail::read_and_write<http::basic_json_body>(
      http_client_core_ptr_old_, header_operator_req(std::move(req))
  );
  if (l_e) {
    http_client_core_ptr_old_->logger_->log(log_loc(), level::warn, "read_and_write error: {}", l_e);
    co_return std::make_tuple(l_e, l_ret);
  }
  if (l_res.result() != boost::beast::http::status::ok) {
    http_client_core_ptr_old_->logger_->log(log_loc(), level::warn, "read_and_write error: {}", l_res.result());
    co_return std::make_tuple(l_e, l_ret);
  }

  auto& l_json = l_res.body();
  try {
    l_ret = l_json["result"]["userid"].get<std::string>();
  } catch (const nlohmann::json::exception& e) {
    l_e = boost::system::error_code{boost::system::errc::bad_message, boost::system::generic_category()};
    http_client_core_ptr_old_->logger_->log(log_loc(), level::warn, "get_user_by_mobile error: {}", e.what());
    co_return std::make_tuple(l_e, l_ret);
  }
  co_return std::make_tuple(l_e, l_ret);
}

boost::asio::awaitable<std::tuple<boost::system::error_code, std::vector<client::attendance_update>>>
client::get_attendance_updatedata(
    const std::string& in_userid, const chrono::local_time_pos& in_work_date

) {
  std::vector<attendance_update> l_ret{};
  boost::beast::http::request<boost::beast::http::string_body> req{
      boost::beast::http::verb::post, fmt::format("/topapi/attendance/getupdatedata?access_token={}", access_token_), 11
  };
  req.body() = nlohmann::json{{"userid", in_userid}, {"work_date", fmt::format("{:%Y-%m-%d}", in_work_date)}}.dump();
  req.set(boost::beast::http::field::content_type, "application/json");
  req.set(boost::beast::http::field::host, http_client_core_ptr_old_->server_ip_);

  auto [l_e, l_res] = co_await http::detail::read_and_write<http::basic_json_body>(
      http_client_core_ptr_old_, header_operator_req(std::move(req))
  );
  if (l_e) {
    http_client_core_ptr_old_->logger_->log(log_loc(), level::warn, "read_and_write error: {}", l_e);
    co_return std::make_tuple(l_e, l_ret);
  }
  if (l_res.result() != boost::beast::http::status::ok) {
    http_client_core_ptr_old_->logger_->log(log_loc(), level::warn, "read_and_write error: {}", l_res.result());
    co_return std::make_tuple(l_e, l_ret);
  }

  auto& l_json = l_res.body();

  try {
    l_ret = l_json["result"]["approve_list"].get<std::vector<attendance_update>>();
  } catch (const nlohmann::json::exception& e) {
    l_e = boost::system::error_code{boost::system::errc::bad_message, boost::system::generic_category()};
    http_client_core_ptr_old_->logger_->log(log_loc(), level::warn, "get_attendance_updatedata error: {}", e.what());
    co_return std::make_tuple(l_e, l_ret);
  }
  co_return std::make_tuple(l_e, l_ret);
}

}  // namespace doodle::dingding