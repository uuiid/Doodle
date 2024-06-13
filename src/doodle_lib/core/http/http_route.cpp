//
// Created by TD on 2024/2/21.
//

#include "http_route.h"

#include <doodle_lib/core/http/http_function.h>
#include <doodle_lib/core/http/http_session_data.h>
namespace doodle::http {

namespace {
void options_function_impl(const http_session_data_ptr& in_handle) {
  auto l_logger = in_handle->logger_;
  auto& l_req   = in_handle->request_parser_->get();
  boost::beast::http::response<boost::beast::http::empty_body> l_response{
      boost::beast::http::status::ok, l_req.version()
  };
  l_response.set(boost::beast::http::field::content_type, "application/json");

  l_response.set(boost::beast::http::field::allow, "POST, GET, PATCH, DELETE, OPTIONS");
  l_response.set(boost::beast::http::field::access_control_allow_origin, "*");
  l_response.set(boost::beast::http::field::access_control_allow_methods, "POST, GET, PATCH, DELETE, OPTIONS");
  l_response.set(
      boost::beast::http::field::access_control_allow_headers, "Origin, X-Requested-With, Content-Type, Accept"
  );

  l_response.keep_alive(l_req.keep_alive());
  l_response.prepare_payload();
  in_handle->seed(std::move(l_response));
}
}  // namespace

http_route::http_route()
    : actions(),
      not_function(std::make_shared<http_function>(
          boost::beast::http::verb::get, "",
          [](const http_session_data_ptr& in_handle) {
            boost::system::error_code l_error_code{ERROR_SERVICE_NOT_FOUND, boost::system::system_category()};
            in_handle->seed_error(boost::beast::http::status::not_found, l_error_code);
          }
      )),
      options_function(std::make_shared<http_function>(boost::beast::http::verb::options, "", options_function_impl))

{}

http_route& http_route::reg(const doodle::http::http_function_ptr in_function) {
  actions[in_function->get_verb()].emplace_back(in_function);
  return *this;
}

http_function_ptr http_route::operator()(
    boost::beast::http::verb in_verb, boost::urls::segments_ref in_segment, const http_session_data_ptr& in_handle
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