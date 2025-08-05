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
class url_route_component_t;
class url_route_component_base_t;
class http_route {
 protected:
  using url_route_component_ptr = std::shared_ptr<url_route_component_base_t>;
  std::vector<std::pair<url_route_component_ptr, http_function_ptr>> url_route_map_;
  http_function_ptr default_function_;

  http_route& reg(url_route_component_ptr&& in_component, const http_function_ptr& in_function);

 public:
  virtual ~http_route() = default;
  /**
   *
   */
  http_route();

  // 注册路由
  template <typename T, typename... Args>
  http_route& reg_t(url_route_component_ptr&& in_component, Args&&... args) {
    return reg(std::forward<url_route_component_ptr>(in_component), std::make_shared<T>(std::forward<Args>(args)...));
  }
  // 路由分发
  virtual http_function_ptr operator()(
      boost::beast::http::verb in_verb, boost::urls::segments_ref in_segment, const session_data_ptr& in_handle
  ) const;
};

}  // namespace doodle::http