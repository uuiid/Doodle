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
  timer_ptr_t timer_ptr_rest_ssl{};

  std::string access_token_;

  std::string app_key;
  std::string app_secret;

  void begin_refresh_token(chrono::seconds in_seconds = chrono::seconds(7200));

  template <typename CompletionHandler>
  struct json_body_impl {
    std::decay_t<CompletionHandler> completion_handler_;
    logger_ptr logger_;
    explicit json_body_impl(CompletionHandler&& in_completion, logger_ptr in_logger)
        : completion_handler_(std::move(in_completion)), logger_(std::move(in_logger)) {}
    // delete copy
    json_body_impl(const json_body_impl&)            = delete;
    json_body_impl& operator=(const json_body_impl&) = delete;
    // move
    json_body_impl(json_body_impl&&)                 = default;
    json_body_impl& operator=(json_body_impl&&)      = default;

    void operator()(boost::system::error_code ec, boost::beast::http::response<boost::beast::http::string_body> res) {
      nlohmann::json l_json{};
      if (ec) {
        logger_->log(log_loc(), level::err, "failed: {}", ec.message());
        boost::asio::post(boost::asio::prepend(completion_handler_, ec, std::move(l_json)));
        return;
      }

      if (res.result() != boost::beast::http::status::ok) {
        logger_->log(log_loc(), level::err, "failed: {}", res.body());
        ec = boost::system::errc::make_error_code(boost::system::errc::bad_message);
        boost::asio::post(boost::asio::prepend(completion_handler_, ec, std::move(l_json)));
        return;
      }

      auto l_json_str = res.body();
      if (!nlohmann::json::accept(l_json_str)) {
        logger_->log(log_loc(), level::err, "failed: {}", l_json_str);
        ec = boost::system::errc::make_error_code(boost::system::errc::bad_message);
        boost::asio::post(boost::asio::prepend(completion_handler_, ec, std::move(l_json)));
        return;
      }
      l_json = nlohmann::json::parse(l_json_str);
      boost::asio::post(boost::asio::prepend(completion_handler_, ec, std::move(l_json)));
    }
  };
  template <typename CompletionHandler>
  struct json_body_impl_access_token {
    std::decay_t<CompletionHandler> completion_handler_;
    bool is_auto_expire_;
    client* http_client_ding;
    logger_ptr logger_;
    explicit json_body_impl_access_token(
        CompletionHandler&& in_completion, bool in_is_auto_expire_, client* in_http_client_ding, logger_ptr in_logger
    )
        : completion_handler_(std::move(in_completion)),
          is_auto_expire_(in_is_auto_expire_),
          http_client_ding(in_http_client_ding),
          logger_(std::move(in_logger)) {}
    // delete copy
    json_body_impl_access_token(const json_body_impl_access_token&)            = delete;
    json_body_impl_access_token& operator=(const json_body_impl_access_token&) = delete;

    // move
    json_body_impl_access_token(json_body_impl_access_token&&)                 = default;
    json_body_impl_access_token& operator=(json_body_impl_access_token&&)      = default;

    void operator()(boost::system::error_code ec, boost::beast::http::response<boost::beast::http::string_body> res) {
      nlohmann::json l_json{};
      if (ec) {
        logger_->log(log_loc(), level::err, "get_user_by_mobile failed: {}", ec.message());
        boost::asio::post(boost::asio::prepend(completion_handler_, ec, std::move(l_json)));
        return;
      }

      if (res.result() != boost::beast::http::status::ok) {
        logger_->log(log_loc(), level::err, "get_user_by_mobile failed: {}", res.body());
        ec = boost::system::errc::make_error_code(boost::system::errc::bad_message);
        boost::asio::post(boost::asio::prepend(completion_handler_, ec, std::move(l_json)));
        return;
      }

      auto l_json_str = res.body();
      if (!nlohmann::json::accept(l_json_str)) {
        logger_->log(log_loc(), level::err, "get_user_by_mobile failed: {}", l_json_str);
        ec = boost::system::errc::make_error_code(boost::system::errc::bad_message);
        boost::asio::post(boost::asio::prepend(completion_handler_, ec, std::move(l_json)));
        return;
      }
      l_json                          = nlohmann::json::parse(l_json_str);
      http_client_ding->access_token_ = l_json["accessToken"].get<std::string>();
      if (is_auto_expire_) http_client_ding->begin_refresh_token(chrono::seconds(l_json["expireIn"].get<int>()));
      boost::asio::post(boost::asio::prepend(completion_handler_, ec, std::move(l_json)));
    }
  };

 public:
  explicit client(boost::asio::ssl::context& in_ctx)
      : http_client_core_ptr_(std::make_shared<https_client_core>(in_ctx, "https://api.dingtalk.com/", "")),
        http_client_core_ptr_old_(std::make_shared<https_client_core>(in_ctx, "https://oapi.dingtalk.com/", "")){};
  ~client() = default;

  // 初始化, 必须调用, 否则无法使用, 获取授权后将自动2小时刷新一次
  void access_token(const std::string& in_app_key, const std::string& in_app_secret, bool in_auto_expire);

  template <typename CompletionHandler>
  void async_access_token(
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
    return boost::asio::async_initiate<CompletionHandler, void(boost::system::error_code, nlohmann::json)>(
        [this, in_auto_expire](auto&& handler, auto in_self, auto in_req) {
          http_client_core_ptr_->async_read<boost::beast::http::response<boost::beast::http::string_body>>(
              in_req,
              json_body_impl_access_token<decltype(handler)>{
                  std::move(handler), in_auto_expire, this, http_client_core_ptr_->logger()
              }
          );
        },
        in_completion, this, req
    );
  }

  template <typename CompletionHandler>
  void get_user_by_mobile(const std::string& in_mobile, CompletionHandler&& in_completion) {
    boost::beast::http::request<boost::beast::http::string_body> req{
        boost::beast::http::verb::post, fmt::format("/topapi/v2/user/getbymobile?access_token={}", access_token_), 11
    };
    req.body() = nlohmann::json{{"mobile", in_mobile}}.dump();
    req.set(boost::beast::http::field::content_type, "application/json");
    return boost::asio::async_initiate<CompletionHandler, void(boost::system::error_code, nlohmann::json)>(
        [this](auto&& handler, auto in_self, auto in_req) {
          http_client_core_ptr_old_->async_read<boost::beast::http::response<boost::beast::http::string_body>>(
              in_req, json_body_impl<decltype(handler)>(std::move(handler), http_client_core_ptr_old_->logger())
          );
        },
        in_completion, this, req
    );
  }
  template <typename CompletionHandler>
  void get_attendance_updatedata(
      const std::string& in_user_id, const chrono::local_time_pos& in_work_date, CompletionHandler&& in_completion
  ) {
    boost::beast::http::request<boost::beast::http::string_body> req{
        boost::beast::http::verb::post, fmt::format("/topapi/attendance/getupdatedata?access_token={}", access_token_),
        11
    };
    req.body() = nlohmann::json{{"userid", in_user_id}, {"work_date", fmt::format("{:%Y-%m-%d}", in_work_date)}}.dump();
    req.set(boost::beast::http::field::content_type, "application/json");
    return boost::asio::async_initiate<CompletionHandler, void(boost::system::error_code, nlohmann::json)>(
        [this](auto&& in_completion, auto in_self, auto in_req) {
          in_self->http_client_core_ptr_old_->async_read<boost::beast::http::response<boost::beast::http::string_body>>(
              in_req,
              json_body_impl<decltype(in_completion)>(std::move(in_completion), http_client_core_ptr_old_->logger())
          );
        },
        in_completion, this, req
    );
  }
};
using client_ptr = std::shared_ptr<client>;

class dingding_company {
 public:
  dingding_company()  = default;
  ~dingding_company() = default;
  struct company_info {
    boost::uuids::uuid corp_id;
    std::string app_key;
    std::string app_secret;
    std::string name;
    client_ptr client_ptr_;
    friend void to_json(nlohmann::json& j, const company_info& p) {
      j["id"]   = fmt::to_string(p.corp_id);
      j["name"] = p.name;
    }
  };
  std::map<boost::uuids::uuid, company_info> company_info_map_;
};

}  // namespace doodle::dingding
