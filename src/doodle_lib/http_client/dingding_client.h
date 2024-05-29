#pragma once
#include <doodle_core/core/http_client_core.h>

#include <doodle_lib/core/http/json_body.h>
#include <doodle_lib/doodle_lib_fwd.h>

namespace doodle::dingding {

class client {
  using https_client_core     = doodle::http::https_client_core;
  using https_client_core_ptr = std::shared_ptr<https_client_core>;

  using timer_t               = boost::asio::steady_timer;
  using timer_ptr_t           = std::shared_ptr<timer_t>;

  https_client_core_ptr http_client_core_ptr_{};
  timer_ptr_t timer_ptr_{};

  std::string access_token_;

  std::string app_key;
  std::string app_secret;

  void begin_refresh_token(chrono::seconds in_seconds = chrono::seconds(7200));

 public:
  explicit client(boost::asio::ssl::context& in_ctx, std::string in_ip, std::string in_port)
      : http_client_core_ptr_(std::make_shared<https_client_core>(in_ctx, std::move(in_ip), std::move(in_port))){};
  ~client() = default;

  // 初始化, 必须调用, 否则无法使用, 获取授权后将自动2分钟刷新一次
  template <typename CompletionHandler>
  void access_token(std::string in_app_key, std::string in_app_secret, CompletionHandler&& in_completion) {
    boost::beast::http::request<boost::beast::http::string_body> req{
        boost::beast::http::verb::post, "/v1.0/oauth2/accessToken", 11
    };
    req.body() = nlohmann::json{{"appKey", in_app_key}, {"appSecret", in_app_secret}}.dump();
    req.set(boost::beast::http::field::content_type, "application/json");

    app_key    = in_app_key;
    app_secret = in_app_secret;
    if (!timer_ptr_) {
      timer_ptr_ = std::make_shared<timer_t>(g_io_context());
    }
    http_client_core_ptr_->async_read<boost::beast::http::response<boost::beast::http::string_body>>(
        req,
        [l_com = std::move(in_completion),
         this](boost::system::error_code ec, boost::beast::http::response<boost::beast::http::string_body> res) {
          if (ec) {
            http_client_core_ptr_->logger()->log(log_loc(), level::err, "access_token failed: {}", ec.message());
            l_com(ec, nlohmann::json{});
            return;
          }

          if (res.result() != boost::beast::http::status::ok) {
            http_client_core_ptr_->logger()->log(log_loc(), level::err, "access_token failed: {}", res.body());
            ec = boost::system::errc::make_error_code(boost::system::errc::bad_message);
            l_com(ec, nlohmann::json{});
            return;
          }

          auto l_json_str = res.body();
          if (!nlohmann::json::accept(l_json_str)) {
            http_client_core_ptr_->logger()->log(log_loc(), level::err, "access_token failed: {}", l_json_str);
            ec = boost::system::errc::make_error_code(boost::system::errc::bad_message);
            l_com(ec, nlohmann::json{});
            return;
          }
          auto l_json = nlohmann::json::parse(l_json_str);
          begin_refresh_token(chrono::seconds(l_json["expireIn"].get<int>()));

          l_com(ec, nlohmann::json::parse(l_json_str));
        }
    );
  }
};

}  // namespace doodle::dingding
