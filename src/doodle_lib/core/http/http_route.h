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
class http_session_data;
class http_websocket_data;
using http_session_data_ptr   = std::shared_ptr<http_session_data>;
using http_function_ptr       = std::shared_ptr<http_function>;
using http_websocket_data_ptr = std::shared_ptr<http_websocket_data>;

class http_route {
 private:
  using map_actin_type = std::vector<http_function_ptr>;
  std::map<boost::beast::http::verb, map_actin_type> actions;
  http_function_ptr not_function;
  http_function_ptr options_function;

 public:
  http_route();
  // 注册路由
  http_route& reg(const http_function_ptr in_function);
  // 路由分发
  http_function_ptr operator()(
      boost::beast::http::verb in_verb, boost::urls::segments_ref in_segment, const session_data_ptr& in_handle
  ) const;
};

}  // namespace doodle::http