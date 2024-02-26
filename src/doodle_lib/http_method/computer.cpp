//
// Created by TD on 2024/2/26.
//

#include "computer.h"

#include "doodle_lib/core/http/http_session_data.h"

#include "core/http/http_session_data.h"
namespace doodle::http {
void computer::list_computers(const entt::handle in_handle) {
  auto &l_req = in_handle.get<http_session_data>().request_parser_->get();
  boost::beast::http::response<boost::beast::http::string_body> l_response{
      boost::beast::http::status::ok, l_req.version()
  };
  l_response.result(boost::beast::http::status::ok);
  l_response.keep_alive(l_req.keep_alive());
  l_response.set(boost::beast::http::field::content_type, "application/json");
  l_response.body() = R"([{"name":"computer1","ip":"192.168.20.1"}])";
  l_response.prepare_payload();
  in_handle.get<http_session_data>().seed(std::move(l_response));
}
void computer::reg(doodle::http::http_route &in_route) {
  in_route
      .reg(std::make_shared<http_function>(boost::beast::http::verb ::get, "v1/computer", [](entt::handle in_handle) {
        boost::asio::post(
            in_handle.get<http_session_data>().stream_->get_executor(), std::bind(&computer::list_computers, in_handle)
        );
      }));
}
}  // namespace doodle::http