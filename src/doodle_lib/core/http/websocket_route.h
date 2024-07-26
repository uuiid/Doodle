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
 public:
  // using call_fun_type = std::function<boost::asio::awaitable<std::string>(http_websocket_data_ptr)>;

  struct call_fun_type {
    using call_t = std::function<boost::asio::awaitable<std::string>(http_websocket_data_ptr)>;
    call_t call{};
    void* user_data_{};
    // call_fun_type() = default;
    // explicit call_fun_type(call_t in_call) : call{std::move(in_call)} {}
    // explicit call_fun_type(call_t in_call, void* in_user_data) : call{std::move(in_call)}, user_data_{in_user_data} {}
  };

 private:
  std::map<std::string, call_fun_type> actions_;

  call_fun_type not_function_;
  static boost::asio::awaitable<std::string> not_function(http_websocket_data_ptr in_data);

 public:
  websocket_route() : not_function_{not_function}, actions_{} {}

  ~websocket_route() = default;

  // 注册路由
  websocket_route& reg(const std::string& in_name, const call_fun_type& in_function);

  // 路由分发
  call_fun_type operator()(const std::string& in_name) const;
};
}  // namespace doodle::http