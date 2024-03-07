//
// Created by TD on 2024/2/26.
//

#include "http_distributed_tasks.h"

#include <doodle_core/core/app_base.h>
#include <doodle_core/core/app_service.h>
#include <doodle_core/lib_warp/boost_fmt_error.h>
#include <doodle_core/logger/logger.h>

#include <doodle_lib/core/http/http_listener.h>
#include <doodle_lib/core/http/http_route.h>
#include <doodle_lib/http_method/computer.h>
#include <doodle_lib/http_method/task_info.h>
namespace doodle::launch {

void reg_func(doodle::http::http_route &in_route) {
  http::computer::reg(in_route);
  http::task_info::reg(in_route);
}

bool http_distributed_tasks::operator()(const argh::parser &in_arh, std::vector<std::shared_ptr<void>> &in_vector) {
  using signal_t      = boost::asio::signal_set;

  auto &l_app         = static_cast<app_service &>(app_base::Get());
  l_app.service_name_ = L"doodle_http_service";
  l_app.display_name_ = L"doodle_http_service";
  l_app.description_  = L"http 服务, 用于在一个线程中执行http任务";
  l_app.command_line_ = L"";

  auto l_signal_ptr   = std::make_shared<signal_t>(g_io_context(), SIGINT, SIGTERM);
  l_signal_ptr->async_wait([](boost::system::error_code ec, int signal) {
    if (ec) {
      default_logger_raw()->log(log_loc(), level::warn, "signal_set_ error: {}", ec);
      return;
    }
    default_logger_raw()->log(log_loc(), level::warn, "收到信号  {}", signal);
    app_base::GetPtr()->stop_app();
  });
  in_vector.emplace_back(l_signal_ptr);
  default_logger_raw()->log(log_loc(), level::warn, "开始服务器");
  auto l_rout_ptr = std::make_shared<http::http_route>();
  reg_func(*l_rout_ptr);
  auto l_listener = std::make_shared<http::http_listener>(g_io_context(), l_rout_ptr);
  l_listener->run();
  in_vector.emplace_back(l_listener);
  return false;
}
}  // namespace doodle::launch