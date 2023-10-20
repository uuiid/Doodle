//
// Created by td_main on 2023/8/15.
//

#include "server_facet.h"

#include <doodle_core/core/doodle_lib.h>
#include <doodle_core/core/program_info.h>
#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/platform/win/windows_alias.h>

#include <doodle_app/app/program_options.h>

#include <doodle_server/render_farm/detail/url_webscoket.h>
#include <doodle_server/render_farm/functional_registration_manager.h>
#include <doodle_server/render_farm/http_listener.h>
#include <spdlog/sinks/stdout_color_sinks.h>
namespace doodle {
bool server_facet::post() {
  win::open_console_window();
  g_logger_ctrl().add_log_sink(std::make_shared<spdlog::sinks::stdout_color_sink_mt>(), "server"s);

  g_ctx().get<program_info>().use_gui_attr(false);
  render_farm::detail::reg_server_websocket{}();
  g_ctx().emplace<doodle::render_farm::http_listener>(g_io_context(), doodle_config::http_port).run();
  return true;
}
}  // namespace doodle