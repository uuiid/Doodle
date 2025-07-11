//
// Created by TD on 25-7-11.
//
//
// Created by TD on 24-9-24.
//
#include "doodle_core/core/app_base.h"

#include "doodle_lib/core/http/http_function.h"
#include "doodle_lib/core/socket_io/socket_io_core.h"
#include "doodle_lib/http_method/model_library/model_library.h"
#include <doodle_lib/core/http/http_listener.h>
#include <doodle_lib/core/http/http_route.h>
#include <doodle_lib/core/scan_win_service.h>
#include <doodle_lib/core/socket_io.h>
#include <doodle_lib/core/socket_io/sid_data.h>

#include <boost/test/unit_test.hpp>
#include <boost/test/unit_test_log.hpp>
using namespace doodle;

namespace {
class test_app : public app_base {
 public:
  bool init() override { return true; }
};
}  // namespace
BOOST_AUTO_TEST_CASE(socket_io_test) {
  test_app l_app{};

  auto l_rout_ptr = std::make_shared<http::http_route>();

  auto l_sid_ctx  = std::make_shared<socket_io::sid_ctx>();
  l_sid_ctx->on("events");
  l_sid_ctx->on({})->on_connect([l_sid_ctx](const std::shared_ptr<socket_io::socket_io_core>& in_data) {
    in_data->emit("auth", in_data->auth_);
    in_data->on_message(
        "message", [in_data](const nlohmann::json& in_json) {
          in_data->emit("message-back", in_json);
        },
        [in_data](const std::vector<std::string>& in_str) { in_data->emit("message-back", in_str); }
    );
  });
  l_sid_ctx->on("/custom")->on_connect([l_sid_ctx](const std::shared_ptr<socket_io::socket_io_core>& in_data) {
    in_data->emit("auth", in_data->auth_);
  });
  socket_io::create_socket_io(*l_rout_ptr, l_sid_ctx);
  http::run_http_listener(g_io_context(), l_rout_ptr, 50025);
  l_app.run();
}
