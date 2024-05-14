#pragma once

#include <doodle_core/core/http_client_core.h>

#include <doodle_lib/core/http/json_body.h>

namespace doodle::kitsu {

class kitsu_response_header_operator {
 public:
  std::string access_token_{};
  std::string refresh_token_{};
  template <typename T, typename ResponeType>
  void operator()(T* in_http_client_core, ResponeType& in_req) {
    in_req.set(boost::beast::http::field::accept, "application/json");
    in_req.set(boost::beast::http::field::content_type, "application/json");
    in_req.set(boost::beast::http::field::host, boost::asio::ip::host_name());
    in_req.set(boost::beast::http::field::authorization, "Bearer " + access_token_);
    in_req.set(boost::beast::http::field::user_agent, BOOST_BEAST_VERSION_STRING);
    in_req.keep_alive(true);
    in_req.prepare_payload();
  }
};

class kitsu_client {
  using http_client_core     = doodle::http::detail::http_client_core<kitsu_response_header_operator>;
  using http_client_core_ptr = std::shared_ptr<http_client_core>;
  http_client_core_ptr http_client_core_ptr_{};

 public:
  kitsu_client(std::string in_ip, std::string in_port)
      : http_client_core_ptr_(std::make_shared<http_client_core>(std::move(in_ip), std::move(in_port))) {}
  ~kitsu_client() = default;

  template <typename CompletionHandler>
  void longin(std::string in_user, std::string in_password, CompletionHandler&& in_completion) {
    boost::beast::http::request<boost::beast::http::string_body> req{
        boost::beast::http::verb::post, "/api/auth/login", 11
    };
    req.body() = nlohmann::json{{"email", in_user}, {"password", in_password}}.dump();
    http_client_core_ptr_->async_read<boost::beast::http::response<http::basic_json_body>>(
        req, boost::asio::bind_executor(
                 g_io_context().get_executor(),
                 [l_com = std::move(in_completion),
                  this](boost::system::error_code ec, boost::beast::http::response<http::basic_json_body> res) {
                   if (ec) {
                     http_client_core_ptr_->logger()->log(log_loc(), level::err, "login failed: {}", ec.message());
                     l_com(ec, nlohmann::json{});
                     return;
                   }
                   if (res.result() != boost::beast::http::status::ok) {
                     http_client_core_ptr_->logger()->log(
                         log_loc(), level::err, "login failed: {}", magic_enum::enum_integer(res.result())
                     );
                     l_com(ec, nlohmann::json{});
                     return;
                   }
                   auto l_json = res.body();
                   if (l_json["login"].get<bool>()) {
                     http_client_core_ptr_->logger()->info("login success");
                     http_client_core_ptr_->response_header_operator().access_token_ =
                         l_json["access_token"].get<std::string>();
                   }
                   l_com(ec, std::move(l_json));
                 }
             )
    );
  }
};
}  // namespace doodle::kitsu