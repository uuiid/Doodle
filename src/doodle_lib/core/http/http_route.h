//
// Created by TD on 2024/2/21.
//
#pragma once
#include <doodle_lib/core/http/http_session_data.h>
#include <doodle_lib/doodle_lib_fwd.h>

#include <boost/dynamic_bitset.hpp>
#include <boost/url.hpp>

#include <map>
#include <memory>
#include <typeindex>

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
  std::map<std::type_index, http_function_ptr> url_route_map_type_;
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
    auto l_type_index = std::type_index(typeid(T));
    auto l_function   = std::make_shared<T>(std::forward<Args>(args)...);
    if (url_route_map_type_.contains(l_type_index)) {
      SPDLOG_WARN("http_route::reg_t: 重复注册路由类型 {}，请检查代码", l_type_index.name());
    } else
      url_route_map_type_[l_type_index] = l_function;
    return reg(std::forward<url_route_component_ptr>(in_component), l_function);
  }
  // 路由分发
  virtual http_function_ptr operator()(
      boost::beast::http::verb in_verb, boost::urls::segments_ref in_segment, const session_data_ptr& in_handle
  ) const;
  template <typename T>
  std::shared_ptr<T> get_function() const {
    auto l_it = url_route_map_type_.find(std::type_index(typeid(T)));
    if (l_it != url_route_map_type_.end()) return std::dynamic_pointer_cast<T>(l_it->second);
    return nullptr;
  }
};

}  // namespace doodle::http