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
  auto &l_app                = static_cast<app_service &>(app_base::Get());
  l_app.service_name_        = L"doodle_scan_win_service";
  l_app.display_name_        = L"doodle_scan_win_service";
  l_app.description_         = L"扫瞄服务器资产并进行确认后提交数据库工具";
  l_app.command_line_        = L"";

  auto scan_win_service_ptr_ = std::make_shared<scan_win_service_t>();
  in_vector.emplace_back(scan_win_service_ptr_);
  boost::asio::post(g_io_context(), [scan_win_service_ptr_]() { scan_win_service_ptr_->start(); });
}

}  // namespace doodle::launch