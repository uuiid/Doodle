//
// Created by TD on 2023/12/29.
//

#include "auto_light_service.h"

#include <doodle_core/core/app_service.h>
#include <doodle_core/database_task/sqlite_client.h>
#include <doodle_core/platform/win/register_file_type.h>

#include <doodle_lib/core/scan_win_service.h>

#include <wil/resource.h>
#include <wil/result.h>
#include <windows.h>
namespace doodle::launch {

bool auto_light_service_t::operator()(const argh::parser &in_arh, std::vector<std::shared_ptr<void>> &in_vector) {
  auto scan_win_service_ptr_ = std::make_shared<scan_win_service_t>();
  in_vector.emplace_back(scan_win_service_ptr_);
  boost::asio::post(g_io_context(), [scan_win_service_ptr_]() { scan_win_service_ptr_->start(); });
  return false;
}

}  // namespace doodle::launch