//
// Created by TD on 2024/3/1.
//

#include "http_working_service.h"

#include <doodle_core/core/app_service.h>
#include <doodle_core/platform/win/register_file_type.h>

#include <doodle_lib/http_client/work.h>

namespace doodle::launch {
bool http_working_service_t::operator()(const argh::parser& in_arh, std::vector<std::shared_ptr<void>>& in_vector) {
  auto& l_app                   = static_cast<app_service&>(app_base::Get());
  l_app.service_name_           = L"doodle_http_client_service";
  l_app.display_name_           = L"doodle_http_client_service";
  l_app.description_            = L"http 客户端, 用于在一个线程中执行http任务";
  l_app.command_line_           = L"";
  auto http_client_service_ptr_ = std::make_shared<http::http_work>();
  in_vector.emplace_back(http_client_service_ptr_);
  boost::asio::post(g_io_context(), [http_client_service_ptr_]() {
    http_client_service_ptr_->run(register_file_type::get_server_address());
  });

  return false;
}
}  // namespace doodle