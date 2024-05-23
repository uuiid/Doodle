//
// Created by TD on 2024/2/27.
//

#pragma once
#include <doodle_core/doodle_core_fwd.h>
namespace doodle::http {
class http_websocket_data;
using http_websocket_data_ptr = std::shared_ptr<http_websocket_data>;
class websocket_route {
  using map_actin_type = std::shared_ptr<std::function<void(const http_websocket_data_ptr&, const nlohmann::json&)>>;
  std::map<std::string, map_actin_type> actions_;

  map_actin_type not_function_;
  static void not_function(const http_websocket_data_ptr& in_data, const nlohmann::json& in_json);

 public:
  websocket_route()
      : not_function_{std::make_shared<std::function<void(const http_websocket_data_ptr&, const nlohmann::json&)>>(
            &not_function
        )},
        actions_{} {}

  ~websocket_route() = default;

  // 注册路由
  websocket_route& reg(const std::string& in_name, const map_actin_type& in_function);

  template <typename T>
  websocket_route& reg(T&& in_fun) {
    return reg(
        T::name, std::make_shared<std::function<void(const http_websocket_data_ptr&, const nlohmann::json&)>>(
                     [l_fun_ptr = std::make_shared<T>(std::move(in_fun)
                      )](const http_websocket_data_ptr& in_ptr, const nlohmann::json& in_data) {
                       auto l_data = in_data;
                       boost::asio::post(boost::asio::prepend(*l_fun_ptr, in_ptr, std::move(l_data)));
                     }
                 )
    );
  }

  // 路由分发
  map_actin_type operator()(const std::string& in_name) const;
};
}  // namespace doodle::http