//
// Created by TD on 2024/2/27.
//

#pragma once
#include <doodle_lib/core/http/http_function.h>
#include <doodle_lib/core/http/http_route.h>
#include <doodle_lib/doodle_lib_fwd.h>
namespace doodle::http {
class task_info {
 public:
  task_info()  = default;
  ~task_info() = default;

  static void post_task(boost::system::error_code in_error_code, entt::handle in_handle);
  static void get_task(boost::system::error_code in_error_code, entt::handle in_handle);
  static void list_task(boost::system::error_code in_error_code, entt::handle in_handle);

  static void reg(http_route& in_route);
};
}  // namespace doodle::http