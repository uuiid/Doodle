#pragma once
#include <doodle_core/core/http_client_core.h>
#include <doodle_core/metadata/time_point_wrap.h>

#include <doodle_lib/core/http/json_body.h>
#include <doodle_lib/doodle_lib_fwd.h>
namespace doodle::dingding {

class client {
  using https_client_core     = doodle::http::https_client_core;
  using https_client_core_ptr = std::shared_ptr<https_client_core>;

  using timer_t               = boost::asio::steady_timer;
  using timer_ptr_t           = std::shared_ptr<timer_t>;

  https_client_core_ptr http_client_core_ptr_{};      // 新版本api
  https_client_core_ptr http_client_core_ptr_old_{};  // 旧版本api
  timer_ptr_t timer_ptr_{};

  std::string access_token_;

  std::string app_key;
  std::string app_secret;

  void begin_refresh_token(chrono::seconds in_seconds = chrono::seconds(7200));

 public:
  explicit client(boost::asio::ssl::context& in_ctx)
      : http_client_core_ptr_(std::make_shared<https_client_core>(in_ctx, "https://api.dingtalk.com/", "443")),
        http_client_core_ptr_old_(std::make_shared<https_client_core>(in_ctx, "https://oapi.dingtalk.com/", "443")){};
  ~client() = default;

  // 初始化, 必须调用, 否则无法使用, 获取授权后将自动2小时刷新一次
  template <typename CompletionHandler>
  void access_token(
      std::string in_app_key, std::string in_app_secret, bool in_auto_expire, CompletionHandler&& in_completion
  ) {
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
        [l_com = std::move(in_completion), in_auto_expire,
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
          auto l_json   = nlohmann::json::parse(l_json_str);
          access_token_ = l_json["accessToken"].get<std::string>();
          if (in_auto_expire) begin_refresh_token(chrono::seconds(l_json["expireIn"].get<int>()));

          l_com(ec, nlohmann::json::parse(l_json_str));
        }
    );
  }

  template <typename CompletionHandler>
  void get_user_by_mobile(const std::string& in_mobile, CompletionHandler&& in_completion) {
    boost::beast::http::request<boost::beast::http::string_body> req{
        boost::beast::http::verb::post, fmt::format("/topapi/v2/user/getbymobile?access_token={}", access_token_), 11
    };
    req.body() = nlohmann::json{{"mobile", in_mobile}}.dump();
    req.set(boost::beast::http::field::content_type, "application/json");

    http_client_core_ptr_old_->async_read<boost::beast::http::response<boost::beast::http::string_body>>(
        req,
        [l_com = std::move(in_completion),
         this](boost::system::error_code ec, boost::beast::http::response<boost::beast::http::string_body> res) {
          if (ec) {
            http_client_core_ptr_->logger()->log(log_loc(), level::err, "get_user_by_mobile failed: {}", ec.message());
            l_com(ec, nlohmann::json{});
            return;
          }

          if (res.result() != boost::beast::http::status::ok) {
            http_client_core_ptr_->logger()->log(log_loc(), level::err, "get_user_by_mobile failed: {}", res.body());
            ec = boost::system::errc::make_error_code(boost::system::errc::bad_message);
            l_com(ec, nlohmann::json{});
            return;
          }

          auto l_json_str = res.body();
          if (!nlohmann::json::accept(l_json_str)) {
            http_client_core_ptr_->logger()->log(log_loc(), level::err, "get_user_by_mobile failed: {}", l_json_str);
            ec = boost::system::errc::make_error_code(boost::system::errc::bad_message);
            l_com(ec, nlohmann::json{});
            return;
          }
          l_com(ec, nlohmann::json::parse(l_json_str));
        }
    );
  }
  template <typename CompletionHandler>
  void get_attendance_updatedata(
      const std::string& in_user_id, const time_point_wrap& in_work_date, CompletionHandler&& in_completion
  ) {
    boost::beast::http::request<boost::beast::http::string_body> req{
        boost::beast::http::verb::post, fmt::format("/attendance/updatedata?access_token={}", access_token_), 11
    };
    req.body() = nlohmann::json{
        {"userid", in_user_id}, {"work_date", fmt::format("{:%Y-%m-%d}", in_work_date.get_local_time())}
    }.dump();
    req.set(boost::beast::http::field::content_type, "application/json");

    http_client_core_ptr_->async_read<boost::beast::http::response<boost::beast::http::string_body>>(
        req,
        [l_com = std::move(in_completion),
         this](boost::system::error_code ec, boost::beast::http::response<boost::beast::http::string_body> res) {
          if (ec) {
            http_client_core_ptr_->logger()->log(
                log_loc(), level::err, "get_attendance_updatedata failed: {}", ec.message()
            );
            l_com(ec, nlohmann::json{});
            return;
          }

          if (res.result() != boost::beast::http::status::ok) {
            http_client_core_ptr_->logger()->log(
                log_loc(), level::err, "get_attendance_updatedata failed: {}", res.body()
            );
            ec = boost::system::errc::make_error_code(boost::system::errc::bad_message);
            l_com(ec, nlohmann::json{});
            return;
          }

          auto l_json_str = res.body();
          if (!nlohmann::json::accept(l_json_str)) {
            http_client_core_ptr_->logger()->log(
                log_loc(), level::err, "get_attendance_updatedata failed: {}", l_json_str
            );
            ec = boost::system::errc::make_error_code(boost::system::errc::bad_message);
            l_com(ec, nlohmann::json{});
            return;
          }
          l_com(ec, nlohmann::json::parse(l_json_str));
        }
    );
  }
};

}  // namespace doodle::dingding
