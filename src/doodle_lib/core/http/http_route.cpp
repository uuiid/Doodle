//
// Created by TD on 2024/2/21.
//

#include "http_route.h"

#include <doodle_lib/core/http/http_function.h>
#include <doodle_lib/core/http/http_session_data.h>
namespace doodle::http {

http_route::http_route()
    : actions(),
      not_function(std::make_shared<http_function>(
          boost::beast::http::verb::get, "",
          [](const http_session_data_ptr& in_handle) {
            boost::system::error_code l_error_code{ERROR_SERVICE_NOT_FOUND, boost::system::system_category()};
            in_handle->seed_error(boost::beast::http::status::not_found, l_error_code);
          }
      )) {}

http_route& http_route::reg(const doodle::http::http_function_ptr in_function) {
  actions[in_function->get_verb()].emplace_back(in_function);
  return *this;
}

http_function_ptr http_route::operator()(
    boost::beast::http::verb in_verb, boost::urls::segments_ref in_segment, const http_session_data_ptr& in_handle
) const {
  auto l_iter = actions.find(in_verb);
  if (l_iter == actions.end()) return not_function;
  for (const auto& i : l_iter->second) {
    if (auto&& [l_m, l_cat] = i->set_match_url(in_segment); l_m) {
      in_handle->capture_ = std::make_shared<capture_t>(std::move(l_cat));
      return i;
    }
  }
  return not_function;
}

}  // namespace doodle::http