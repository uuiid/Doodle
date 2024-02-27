//
// Created by TD on 2024/2/26.
//

#pragma once
#include <doodle_lib/core/http/http_function.h>
#include <doodle_lib/core/http/http_route.h>
namespace doodle::http {

class computer {
 public:
  computer()  = default;
  ~computer() = default;

  // get 方法, 列出所有的注册计算机
  static void list_computers(boost::system::error_code in_error_code, const entt::handle in_handle);

  static void reg(http_route& in_route);
};

}  // namespace doodle::http