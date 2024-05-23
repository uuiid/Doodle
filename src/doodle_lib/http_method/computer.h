//
// Created by TD on 2024/2/26.
//

#pragma once
#include <doodle_lib/core/http/http_function.h>
#include <doodle_lib/core/http/http_route.h>
namespace doodle::http {
enum class computer_websocket_fun { set_state, set_task, logger };
NLOHMANN_JSON_SERIALIZE_ENUM(
    computer_websocket_fun, {{computer_websocket_fun::set_state, "set_state"},
                             {computer_websocket_fun::set_task, "set_task"},
                             {computer_websocket_fun::logger, "logger"}}
);
class computer {
 public:
  computer()  = default;
  ~computer() = default;

   // get 方法, 列出所有的注册计算机
  static void list_computers(boost::system::error_code in_error_code, const http_session_data_ptr& in_handle);
  // websocket 方法, 注册计算机
  static void reg_computer(boost::system::error_code in_error_code, const http_websocket_data_ptr& in_handle);

  static void reg(http_route& in_route);
};

}  // namespace doodle::http