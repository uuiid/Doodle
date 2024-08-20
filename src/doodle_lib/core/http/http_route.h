//
// Created by TD on 2024/2/21.
//
#pragma once
#include <doodle_core/doodle_core_fwd.h>

#include <doodle_lib/core/http/http_session_data.h>

#include <boost/dynamic_bitset.hpp>
#include <boost/url.hpp>

namespace doodle::http {
class http_function;
using http_function_ptr = std::shared_ptr<http_function>;
class websocket_route;
using websocket_route_ptr = std::shared_ptr<websocket_route>;

class http_route {
 private:
  using map_actin_type = std::vector<http_function_ptr>;
  std::map<boost::beast::http::verb, map_actin_type> actions;
  http_function_ptr not_function;
  http_function_ptr options_function;

 public:
  http_route();
  explicit http_route(http_function_ptr in_not_function, http_function_ptr in_options_function)
      : not_function(in_not_function), options_function(in_options_function){};
  // 注册路由
  http_route& reg(const http_function_ptr in_function);
  // 路由分发
  http_function_ptr operator()(
      boost::beast::http::verb in_verb, boost::urls::segments_ref in_segment, const session_data_ptr& in_handle
  ) const;
};
}  // namespace doodle::http