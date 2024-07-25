//
// Created by TD on 2024/2/27.
//

#pragma once
#include <doodle_core/doodle_core_fwd.h>

#include <boost/asio/prepend.hpp>

namespace doodle::http {
namespace detail {
class http_websocket_data;
}

using http_websocket_data_ptr = std::shared_ptr<detail::http_websocket_data>;

class websocket_route {
  using call_fun_type  = std::function<boost::asio::awaitable<std::string>(http_websocket_data_ptr)>;
  using map_actin_type = std::shared_ptr<call_fun_type>;
  std::map<std::string, map_actin_type> actions_;

  map_actin_type not_function_;
  static boost::asio::awaitable<std::string> not_function(http_websocket_data_ptr in_data);

public:
  websocket_route()
    : not_function_{std::make_shared<call_fun_type>(
        &not_function
      )},
      actions_{} {
  }

  ~websocket_route() = default;

  // 注册路由
  websocket_route& reg(const std::string& in_name, const map_actin_type& in_function);

  template <typename T>
  websocket_route& reg(std::string in_name, T&& in_fun) {
    return reg(
      in_name,
      std::make_shared<call_fun_type>(
        std::move(in_fun)
      )
    );
  }

  // 路由分发
  map_actin_type operator()(const std::string& in_name) const;
};
} // namespace doodle::http