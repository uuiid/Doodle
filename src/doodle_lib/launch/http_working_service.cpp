//
// Created by TD on 2024/3/1.
//

#include "http_working_service.h"

#include <doodle_core/core/app_service.h>
#include <doodle_core/lib_warp/boost_fmt_error.h>
#include <doodle_core/platform/win/register_file_type.h>

#include <doodle_lib/http_client/work.h>

namespace doodle::launch {
bool http_working_service_t::operator()(const argh::parser& in_arh, std::vector<std::shared_ptr<void>>& in_vector) {
  using signal_set              = boost::asio::signal_set;
  auto& l_app                   = static_cast<app_service&>(app_base::Get());
  l_app.service_name_           = L"doodle_http_client_service";
  l_app.display_name_           = L"doodle_http_client_service";
  l_app.description_            = L"http 客户端, 用于在一个线程中执行http任务";
  l_app.command_line_           = L"";
  auto http_client_service_ptr_ = std::make_shared<http::http_work>();
  in_vector.emplace_back(http_client_service_ptr_);
  auto l_signal_set_ = std::make_shared<signal_set>(g_io_context(), SIGINT, SIGTERM);
  l_signal_set_->async_wait([http_client_service_ptr_](boost::system::error_code in_error_code, int in_signal) {
    if (in_error_code) {
      default_logger_raw()->log(log_loc(), level::err, "signal_set error: {}", in_error_code);
      return;
    }
    if (in_signal == SIGINT || in_signal == SIGTERM) {
      http_client_service_ptr_->stop();
    }
    for (int l = 0; l < 10; ++l) {
      g_io_context().poll_one();
    }
    app_base::Get().stop_app();
  });

  boost::asio::post(g_io_context(), [http_client_service_ptr_]() {
    http_client_service_ptr_->run(register_file_type::get_server_address());
  });

  return false;
}
}  // namespace doodle::launch