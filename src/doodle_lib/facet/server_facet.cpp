//
// Created by td_main on 2023/8/15.
//

#include "server_facet.h"

#include <doodle_core/core/doodle_lib.h>
#include <doodle_core/core/program_info.h>
#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/platform/win/windows_alias.h>

#include <doodle_app/app/program_options.h>

#include <doodle_lib/render_farm/udp_server.h>
#include <doodle_lib/render_farm/working_machine.h>

#include "boost/asio/executor_work_guard.hpp"

#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>
namespace doodle {
bool server_facet::post() {
  win::open_console_window();
  g_logger_ctrl().add_log_sink(std::make_shared<spdlog::sinks::stdout_color_sink_mt>(), "server"s);

  g_ctx().get<program_info>().use_gui_attr(false);
  g_ctx()
      .emplace<doodle::render_farm::working_machine_ptr>(
          std::make_shared<doodle::render_farm::working_machine>(g_io_context(), doodle_config::http_port)
      )
      ->config_server();
  g_ctx().emplace<doodle::udp_server_ptr>(std::make_shared<udp_server>(g_io_context()))->run();
  return true;
}
}  // namespace doodle