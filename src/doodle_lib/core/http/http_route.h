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
  /**
   *
   */
  http_route();
  /**
   * @param in_create_proxy_factory 代理工厂
   *
   * @note 这个创建路由方式暗示使用代理转发, 会将请求转发到代理服务器, 并且没有 404 响应
   */
  explicit http_route(std::function<boost::asio::awaitable<tcp_stream_type_ptr>()> in_create_proxy_factory)
      : create_proxy_factory_(in_create_proxy_factory){};
  // 注册路由
  http_route& reg(const http_function_ptr in_function);
  // 路由分发
  http_function_ptr operator()(
      boost::beast::http::verb in_verb, boost::urls::segments_ref in_segment, const session_data_ptr& in_handle
  ) const;

  std::function<boost::asio::awaitable<tcp_stream_type_ptr>()> create_proxy_factory_;
};
}  // namespace doodle::http