//
// Created by TD on 24-4-22.
//

#pragma once
#include <doodle_lib/core/http/http_function.h>
#include <doodle_lib/core/http/http_route.h>
namespace doodle::http {
class file_exists {
 public:
  file_exists()  = default;
  ~file_exists() = default;

  static void file_exists_fun(boost::system::error_code in_error_code, const entt::handle in_handle);

  static void reg(http_route& in_route);
};

}  // namespace doodle::http