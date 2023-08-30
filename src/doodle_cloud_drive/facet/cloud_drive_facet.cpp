//
// Created by td_main on 2023/7/5.
//

#include "cloud_drive_facet.h"

#include "doodle_core/core/app_base.h"
#include "doodle_core/core/global_function.h"
#include <doodle_core/core/doodle_lib.h>
#include <doodle_core/core/file_sys.h>

#include <doodle_app/app/app_command.h>
#include <doodle_app/app/program_options.h>

#include <doodle_cloud_drive/cloud/cloud_provider_registrar.h>
#include <doodle_cloud_drive/cloud/directory_watcher.h>
#include <memory>
#include <nlohmann/json.hpp>
namespace doodle {

bool cloud_drive_facet::post() {
  bool l_r{};
  auto l_str = FSys::from_quotation_marks(g_ctx().get<program_options>().arg(cloud_drive_arg::name).str());
  if (l_str.empty()) {
    return l_r;
  }
  cloud_drive_arg l_arg{};

  try {
    l_arg = nlohmann::json::parse(FSys::ifstream{l_str}).get<cloud_drive_arg>();
  } catch (const nlohmann::json::exception& e) {
    DOODLE_LOG_ERROR("解析配置失败 {}", e.what());
    return l_r;
  }

  registrar_ = std::make_shared<cloud_provider_registrar>(l_arg.root_path, l_arg.server_path);
  l_r        = true;
  signals_.async_wait([this](boost::system::error_code, int) {
    DOODLE_LOG_INFO("收到退出信号");
    work_guard_.reset();
    app_base::Get().stop_app();
    DOODLE_LOG_INFO("停止所以循环");
  });
  boost::asio::post(g_io_context(), [this, l_arg]() { registrar_->create_placeholder(l_arg.server_path); });
  return l_r;
}
void cloud_drive_facet::add_program_options() { g_ctx().get<program_options>().arg.add_param(cloud_drive_arg::name); }

}  // namespace doodle