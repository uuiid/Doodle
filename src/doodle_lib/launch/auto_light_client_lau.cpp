//
// Created by TD on 2024/3/1.
//

#include "auto_light_client_lau.h"

#include <doodle_core/core/app_service.h>
#include <doodle_core/lib_warp/boost_fmt_error.h>
#include <doodle_core/platform/win/register_file_type.h>

#include <doodle_lib/http_client/work.h>

namespace doodle::launch {
bool auto_light_client_lau::operator()(const argh::parser& in_arh, std::vector<std::shared_ptr<void>>& in_vector) {
  using signal_set              = boost::asio::signal_set;
  auto http_client_service_ptr_ = std::make_shared<http::http_work>();
  in_vector.emplace_back(http_client_service_ptr_);

  http_client_service_ptr_->run(register_file_type::get_server_address());

  return false;
}
}  // namespace doodle::launch