#pragma once

#include <doodle_lib/core/http/http_function.h>
#include <doodle_lib/core/http/http_route.h>

#include <memory>
#include <string>

namespace doodle::http {

class test_fun : public http_function {
  using base_type = http_function;

 protected:
  std::string root_{};
  std::string db_path_{};
  std::uint16_t port_{0};

 public:
  test_fun(std::string in_root, std::string in_db_path, std::uint16_t in_port)
      : root_(std::move(in_root)), db_path_(std::move(in_db_path)), port_(in_port) {}
};

// /api/doodle/test/health
DOODLE_HTTP_FUN_C(test_health, test_fun)
using test_fun::test_fun;
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_ROUTE("/api/doodle/test/health")
DOODLE_HTTP_FUN_END()

// /api/doodle/test/stop
DOODLE_HTTP_FUN_C(test_stop, test_fun)
using test_fun::test_fun;
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_ROUTE("/api/doodle/test/stop")
DOODLE_HTTP_FUN_END()

http_route_ptr create_test_route(
    std::string in_root, std::string in_db_path, std::uint16_t in_port
);

}  // namespace doodle::http
