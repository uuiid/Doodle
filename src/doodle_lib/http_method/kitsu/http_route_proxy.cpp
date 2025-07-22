//
// Created by TD on 24-10-10.
//

#include "http_route_proxy.h"

#include <doodle_lib/core/http/http_function.h>
#include <doodle_lib/http_method/kitsu/kitsu.h>
namespace doodle::http::kitsu {
http_function_ptr http_route_proxy::operator()(
    boost::beast::http::verb in_verb, boost::urls::segments_ref in_segment, const session_data_ptr& in_handle
) const {
  auto l_ptr = http_route::operator()(in_verb, in_segment, in_handle);
  if (!l_ptr && !in_segment.empty() && in_segment.front() == "api") return my_not_function;
  if (l_ptr == nullptr) {
    return in_verb == boost::beast::http::verb::head ? head_file_ : get_file_;
  }
  return l_ptr;
}

http_route_proxy& http_route_proxy::reg_proxy(const http_function_ptr in_function) {
  proxy_urls.emplace_back(in_function);
  return *this;
}
void http_route_proxy::reg_front_end(const http_function_ptr in_get_index, const http_function_ptr in_head_file) {
  get_file_  = in_get_index;
  head_file_ = in_head_file;
}

}  // namespace doodle::http::kitsu