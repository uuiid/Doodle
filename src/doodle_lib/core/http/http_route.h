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
class http_function_base_t;
using http_function_ptr = std::shared_ptr<http_function_base_t>;
class websocket_route;
using websocket_route_ptr = std::shared_ptr<websocket_route>;

class http_route {
 protected:
  using map_actin_type = std::vector<http_function_ptr>;
  std::map<boost::beast::http::verb, map_actin_type> actions;
  http_function_ptr not_function;
  http_function_ptr options_function;

 public:
  virtual ~http_route() = default;
  /**
   *
   */
  http_route();

  // 注册路由
  http_route& reg(const http_function_ptr in_function);
  // 路由分发
  virtual http_function_ptr operator()(
      boost::beast::http::verb in_verb, boost::urls::segments_ref in_segment, const session_data_ptr& in_handle
  ) const;

  virtual boost::asio::awaitable<tcp_stream_type_ptr> create_proxy() const { co_return nullptr; }
};

}  // namespace doodle::http