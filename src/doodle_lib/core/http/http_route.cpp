//
// Created by TD on 2024/2/21.
//

#include "http_route.h"

#include <doodle_lib/core/http/http_function.h>
#include <doodle_lib/core/http/http_session_data.h>

#include "../../../../build/Ninja_RelWithDebInfo/vcpkg_installed/x64-windows/include/boost/preprocessor/tuple/to_seq.hpp"
namespace doodle::http {

DOODLE_HTTP_FUN(http_not_function)
http_not_function() = default;
DOODLE_HTTP_FUN_END()

http_route::http_route() : default_function_(std::make_shared<http_not_function>()) {}

http_route& http_route::reg(url_route_component_ptr&& in_component, const http_function_ptr& in_function) {
  url_route_map_.emplace_back(std::move(in_component), in_function);
  return *this;
}

http_function_ptr http_route::operator()(
    boost::beast::http::verb in_verb, boost::urls::segments_ref in_segment, const session_data_ptr& in_handle
) const {
  for (const auto& [i, l_data] : url_route_map_) {
    if (auto&& [l_m, l_cat] = i->set_match_url(in_segment, l_data); l_m) {
      return l_cat;
    }
  }
  return default_function_;
}

}  // namespace doodle::http