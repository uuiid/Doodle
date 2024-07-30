//
// Created by TD on 2024/2/26.
//

#include "auto_light_server_lau.h"

#include <doodle_core/core/app_base.h>
#include <doodle_core/core/app_service.h>
#include <doodle_core/lib_warp/boost_fmt_error.h>
#include <doodle_core/logger/logger.h>

#include <doodle_lib/core/http/http_listener.h>
#include <doodle_lib/core/http/http_route.h>
#include <doodle_lib/http_method/computer.h>
#include <doodle_lib/http_method/task_info.h>
#include <doodle_lib/http_method/task_server.h>

namespace doodle::launch {
void reg_func(doodle::http::http_route& in_route) {
  http::computer_reg(in_route);
  http::task_info_reg(in_route);
}

bool auto_light_server_lau::operator()(const argh::parser& in_arh, std::vector<std::shared_ptr<void>>& in_vector) {
  default_logger_raw()->log(log_loc(), level::warn, "开始服务器");

  default_logger_raw()->log(log_loc(), level::warn, "开始加载快照");

  auto l_rout_ptr = std::make_shared<http::http_route>();
  default_logger_raw()->log(log_loc(), level::warn, "开始路由");
  reg_func(*l_rout_ptr);
  http::run_http_listener(g_io_context(), l_rout_ptr, 50023);
  if (!g_ctx().contains<http::task_server>()) {
    g_ctx().emplace<http::task_server>();
  }
  g_ctx().get<http::task_server>().run();
  default_logger_raw()->log(log_loc(), level::warn, "启动侦听器");
  return false;
}
}  // namespace doodle::launch