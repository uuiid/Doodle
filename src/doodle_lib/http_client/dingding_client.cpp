#include "dingding_client.h"

#include <doodle_core/core/app_base.h>
#include <doodle_core/lib_warp/boost_fmt_beast.h>

namespace doodle::dingding {
boost::asio::awaitable<void> client::begin_refresh_token() {
  while (auto_expire_) {
    if ((co_await boost::asio::this_coro::cancellation_state).cancelled() != boost::asio::cancellation_type::none) {
      co_return;
    }

    boost::beast::http::request<boost::beast::http::string_body> req{
        boost::beast::http::verb::post, "/v1.0/oauth2/accessToken", 11
    };
    req.body() = nlohmann::json{{"appKey", app_key}, {"appSecret", app_secret}}.dump();
    req.set(boost::beast::http::field::content_type, "application/json");
    auto [l_e, l_res] = co_await http::detail::read_and_write<boost::beast::http::string_body>(
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
    if (!nlohmann::json::accept(l_res.body())) {
      http_client_core_ptr_->logger_->log(log_loc(), level::warn, "read_and_write error: {}", l_res.body());
      co_return;
    }

    nlohmann::json l_json     = nlohmann::json::parse(l_res.body());
    access_token_             = l_json["accessToken"].get<std::string>();
    chrono::seconds l_seconds = chrono::seconds(l_json["expireIn"].get<int>());
    if (!auto_expire_) co_return;

    timer_ptr_->expires_after(l_seconds);
    auto [l_ex] = co_await timer_ptr_->async_wait();
    if (l_ex) {
      http_client_core_ptr_->logger_->log(log_loc(), level::warn, "timer_ptr_ error: {}", l_ex);
      co_return;
    }
  }
}
void client::access_token(const std::string& in_app_key, const std::string& in_app_secret, bool in_auto_expire) {
  app_key    = in_app_key;
  app_secret = in_app_secret;
  auto_expire_ = in_auto_expire;

  boost::asio::co_spawn(
      http_client_core_ptr_->get_executor(), begin_refresh_token(),
      boost::asio::bind_cancellation_slot(app_base::Get().on_cancel.slot(), boost::asio::detached)
  );
}

}  // namespace doodle::dingding