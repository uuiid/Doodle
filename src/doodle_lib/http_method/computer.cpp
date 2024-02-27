//
// Created by TD on 2024/2/26.
//

#include "computer.h"

#include <doodle_core/lib_warp/boost_fmt_error.h>
#include <doodle_core/metadata/computer.h>

#include "doodle_lib/core/http/http_session_data.h"
#include <doodle_lib/core/http/json_body.h>
namespace doodle::http {
void computer::list_computers(boost::system::error_code in_error_code, const entt::handle in_handle) {
  std::vector<doodle::computer> l_computers{};
  for (auto &&[l_e, l_c] : g_reg()->view<doodle::computer>().each()) {
    l_computers.emplace_back(l_c);
  }
  nlohmann::json l_json = l_computers;
  auto &l_req           = in_handle.get<http_session_data>().request_parser_->get();
  boost::beast::http::response<boost::beast::http::string_body> l_response{
      boost::beast::http::status::ok, l_req.version()
  };
  l_response.result(boost::beast::http::status::ok);
  l_response.keep_alive(l_req.keep_alive());
  l_response.set(boost::beast::http::field::content_type, "application/json");
  l_response.body() = l_json.dump();
  l_response.prepare_payload();
  in_handle.get<http_session_data>().seed(std::move(l_response));
}

void computer::reg(doodle::http::http_route &in_route) {
  in_route.reg(std::make_shared<http_function>(
      boost::beast::http::verb ::get, "v1/computer",
      session::http_method_web_socket{
          "v1/computer", boost::asio::bind_executor(g_io_context(), &computer::list_computers)
      }
  ));
}
}  // namespace doodle::http