//
// Created by TD on 2024/2/21.
//

#include "http_route.h"

#include <doodle_lib/core/http/http_function.h>
#include <doodle_lib/core/http/http_session_data.h>
namespace doodle::http {

namespace {

DOODLE_HTTP_FUN(options_function, options, "", http_function)
boost::asio::awaitable<boost::beast::http::message_generator> callback(session_data_ptr in_handle) override {
  boost::beast::http::response<boost::beast::http::empty_body> l_response{
      boost::beast::http::status::ok, in_handle->version_
  };
  l_response.set(boost::beast::http::field::content_type, "application/json");

  l_response.set(boost::beast::http::field::allow, "GET, POST, PUT, PATCH, DELETE, OPTIONS");
  l_response.set(boost::beast::http::field::access_control_allow_origin, "*");
  l_response.set(boost::beast::http::field::access_control_allow_methods, "GET, POST, PUT, PATCH, DELETE, OPTIONS");
  l_response.set(
      boost::beast::http::field::access_control_allow_headers,
      "Authorization, Origin, X-Requested-With, Content-Type, Accept"
  );

  l_response.keep_alive(in_handle->keep_alive_);
  l_response.prepare_payload();
  co_return l_response;
}
DOODLE_HTTP_FUN_END()

DOODLE_HTTP_FUN(not_function, get, "", http_function)
boost::asio::awaitable<boost::beast::http::message_generator> callback(session_data_ptr in_handle) override {
  static boost::system::error_code l_error_code{ERROR_SERVICE_NOT_FOUND, boost::system::system_category()};
  co_return in_handle->make_error_code_msg(boost::beast::http::status::not_found, l_error_code);
}
DOODLE_HTTP_FUN_END()

}  // namespace

http_route::http_route()
    : not_function(std::make_shared<not_function_get>()),
      options_function(std::make_shared<options_function_options>())

{}

http_route& http_route::reg(const doodle::http::http_function_ptr in_function) {
  actions[in_function->get_verb()].emplace_back(in_function);
  return *this;
}

http_function_ptr http_route::operator()(
    boost::beast::http::verb in_verb, boost::urls::segments_ref in_segment, const session_data_ptr& in_handle
) const {
  auto l_iter = actions.find(in_verb);

  auto l_ret  = not_function;
  if (in_verb == boost::beast::http::verb::options) {
    l_ret = options_function;
  }
  if (l_iter == actions.end()) return l_ret;
  for (const auto& i : l_iter->second) {
    if (auto&& [l_m, l_cat] = i->set_match_url(in_segment); l_m) {
      in_handle->capture_ = std::make_shared<capture_t>(std::move(l_cat));
      return i;
    }
  }
  return l_ret;
}

}  // namespace doodle::http