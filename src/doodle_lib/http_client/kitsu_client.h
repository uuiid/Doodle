#pragma once

#include <doodle_core/core/http_client_core.h>

#include <doodle_lib/core/http/json_body.h>

#include <boost/algorithm/string.hpp>

namespace doodle::kitsu {
class kitsu_client;

class kitsu_request_header_operator {
 public:
  kitsu_client* kitsu_client_ptr_{};
  template <typename T, typename ResponeType>
  void operator()(T* in_http_client_core, ResponeType& in_req);
};

class kitsu_response_header_operator {
 public:
  kitsu_client* kitsu_client_ptr_{};
  template <typename T, typename ResponeType>
  void operator()(T* in_http_client_core, ResponeType& in_req);
};

class kitsu_client {
  using http_client_core =
      doodle::http::detail::http_client_core<kitsu_request_header_operator, kitsu_response_header_operator>;
  using http_client_core_ptr = std::shared_ptr<http_client_core>;
  http_client_core_ptr http_client_core_ptr_{};
  friend class kitsu_request_header_operator;
  friend class kitsu_response_header_operator;
  std::string access_token_{};
  std::string refresh_token_{};
  std::string session_cookie_{};

 public:
  explicit kitsu_client(std::string in_ip, std::string in_port)
      : http_client_core_ptr_(std::make_shared<http_client_core>(std::move(in_ip), std::move(in_port))) {
    http_client_core_ptr_->request_header_operator().kitsu_client_ptr_  = this;
    http_client_core_ptr_->response_header_operator().kitsu_client_ptr_ = this;
  }
  ~kitsu_client() = default;
 
  template <typename CompletionHandler>
  auto authenticated(std::string in_token, CompletionHandler&& in_completion) {
    boost::beast::http::request<boost::beast::http::string_body> req{
        boost::beast::http::verb::get, "/api/auth/authenticated", 11
    };
    access_token_ = in_token;

    boost::asio::async_initiate<void(boost::system::error_code, nlohmann::json)>(
      
    );

    http_client_core_ptr_->async_read<boost::beast::http::response<boost::beast::http::string_body>>(
        req,
        boost::asio::bind_executor(
            g_io_context().get_executor(),
            [l_com = std::move(in_completion),
             this](boost::system::error_code ec, boost::beast::http::response<boost::beast::http::string_body> res) {
              if (ec) {
                http_client_core_ptr_->logger()->log(log_loc(), level::err, "authenticated failed: {}", ec.message());
                l_com(ec, nlohmann::json{});
                return;
              }

              if (res.result() != boost::beast::http::status::ok) {
                http_client_core_ptr_->logger()->log(
                    log_loc(), level::err, "authenticated failed: {}", magic_enum::enum_integer(res.result())
                );
                ec = boost::system::errc::make_error_code(boost::system::errc::bad_message);
                l_com(ec, nlohmann::json{});
                return;
              }

              auto l_json_str = res.body();
              if (!nlohmann::json::accept(l_json_str)) {
                http_client_core_ptr_->logger()->log(log_loc(), level::err, "authenticated failed: {}", l_json_str);
                ec = boost::system::errc::make_error_code(boost::system::errc::bad_message);
                l_com(ec, nlohmann::json{});
                return;
              }
              auto l_json = nlohmann::json::parse(l_json_str);
              l_com(ec, std::move(l_json));
            }
        )
    );
  }

  template <typename CompletionHandler>
  void get_task(std::string in_uuid, CompletionHandler&& in_completio) {
    boost::beast::http::request<boost::beast::http::empty_body> req{
        boost::beast::http::verb::get, fmt::format("/api/data/tasks/{}/full", in_uuid), 11
    };
    http_client_core_ptr_->async_read<boost::beast::http::response<boost::beast::http::string_body>>(
        req,
        boost::asio::bind_executor(
            g_io_context().get_executor(),
            [l_com = std::move(in_completio),
             this](boost::system::error_code ec, boost::beast::http::response<boost::beast::http::string_body> res) {
              if (ec) {
                http_client_core_ptr_->logger()->log(log_loc(), level::err, "get task failed: {}", ec.message());
                l_com(ec, nlohmann::json{});
                return;
              }

              if (res.result() != boost::beast::http::status::ok) {
                http_client_core_ptr_->logger()->log(
                    log_loc(), level::err, "get task failed: {}", magic_enum::enum_integer(res.result())
                );
                ec = boost::system::errc::make_error_code(boost::system::errc::bad_message);
                l_com(ec, nlohmann::json{});
                return;
              }

              if (!nlohmann::json::accept(res.body())) {
                http_client_core_ptr_->logger()->log(log_loc(), level::err, "get task failed: {}", res.body());
                ec = boost::system::errc::make_error_code(boost::system::errc::bad_message);
                l_com(ec, nlohmann::json{});
                return;
              }
              auto l_json = nlohmann::json::parse(res.body());
              l_com(ec, l_json);
            }
        )
    );
  }
};
template <typename T, typename ResponeType>
void kitsu_request_header_operator::operator()(T* in_http_client_core, ResponeType& in_req) {
  in_req.set(boost::beast::http::field::accept, "application/json");
  in_req.set(boost::beast::http::field::content_type, "application/json");
  in_req.set(boost::beast::http::field::host, boost::asio::ip::host_name());
  in_req.set(boost::beast::http::field::authorization, "Bearer " + kitsu_client_ptr_->access_token_);
  if (!kitsu_client_ptr_->session_cookie_.empty()) {
    in_req.set(boost::beast::http::field::cookie, kitsu_client_ptr_->session_cookie_ + ";");
  }
  in_req.set(boost::beast::http::field::user_agent, BOOST_BEAST_VERSION_STRING);
  in_req.keep_alive(true);
  in_req.prepare_payload();
}
template <typename T, typename ResponeType>
void kitsu_response_header_operator::operator()(T* in_http_client_core, ResponeType& in_req) {
  auto l_set_cookie = in_req[boost::beast::http::field::set_cookie];
  if (!l_set_cookie.empty()) {
    std::vector<std::string> l_cookie;
    boost::split(l_cookie, l_set_cookie, boost::is_any_of(";"));
    for (auto& l_item : l_cookie) {
      if (l_item.starts_with("session=")) {
        kitsu_client_ptr_->session_cookie_ = l_item;
      }
    }
  }
}
}  // namespace doodle::kitsu