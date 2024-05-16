//
// Created by TD on 24-4-23.
//

#include "file_exists_launch.h"

#include <doodle_core/core/app_base.h>
#include <doodle_core/core/app_service.h>
#include <doodle_core/lib_warp/boost_fmt_error.h>
#include <doodle_core/logger/logger.h>

#include <doodle_lib/core/http/http_listener.h>
#include <doodle_lib/core/http/http_route.h>
#include <doodle_lib/http_method/file_exists.h>

namespace doodle::launch {
namespace {
void reg_func(doodle::http::http_route &in_route) { http::file_exists::reg(in_route); }
}  // namespace
bool file_exists_launch_t::operator()(const argh::parser &in_arh, std::vector<std::shared_ptr<void>> &in_vector) {
  // auto &l_app         = static_cast<app_service &>(app_base::Get());
  // l_app.service_name_ = L"doodle_http_file_exists";
  // l_app.display_name_ = L"doodle_http_file_exists";
  // l_app.description_  = L"http 服务, 用于在一个线程中确认文件的存在性";
  // l_app.command_line_ = L"";

  default_logger_raw()->log(log_loc(), level::warn, "开始服务器");

  auto l_rout_ptr = std::make_shared<http::http_route>();
  default_logger_raw()->log(log_loc(), level::warn, "开始路由");
  reg_func(*l_rout_ptr);
  auto l_listener = std::make_shared<http::http_listener>(g_io_context(), l_rout_ptr);
  default_logger_raw()->log(log_loc(), level::warn, "启动侦听器");
  l_listener->run();
  in_vector.emplace_back(l_listener);
  return false;
}

}  // namespace doodle::launch