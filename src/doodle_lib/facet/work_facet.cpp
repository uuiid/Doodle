//
// Created by td_main on 2023/8/21.
//

#include "work_facet.h"

#include <doodle_core/core/app_base.h>
#include <doodle_core/core/doodle_lib.h>
#include <doodle_core/core/program_info.h>
#include <doodle_core/doodle_core_fwd.h>

#include <doodle_app/app/program_options.h>

#include <doodle_lib/render_farm/work.h>
#include <doodle_lib/render_farm/working_machine.h>

#include "boost/asio/executor_work_guard.hpp"

#include <spdlog/sinks/stdout_color_sinks.h>
namespace doodle {
bool work_facet::post() {
  signal_set_ = std::make_shared<signal_set>(g_io_context(), SIGINT, SIGTERM);
  signal_set_->async_wait([&](boost::system::error_code ec, int signal) {
    if (ec) {
      DOODLE_LOG_ERROR("signal_set_ error: {}", ec.message());
      return;
    }
    DOODLE_LOG_INFO("signal_set_ signal: {}", signal);
    app_base::Get().stop_app();
  });

  win::open_console_window();
  g_logger_ctrl().add_log_sink(std::make_shared<spdlog::sinks::stdout_color_sink_mt>(), "work"s);

  g_ctx().get<program_info>().use_gui_attr(false);
  guard_ = std::make_shared<decltype(guard_)::element_type>(boost::asio::make_work_guard(g_io_context()));

  g_ctx().emplace<doodle::render_farm::work_ptr>(std::make_shared<render_farm::work>())->run();
  g_ctx()
      .emplace<doodle::render_farm::working_machine_ptr>(
          std::make_shared<doodle::render_farm::working_machine>(g_io_context(), doodle_config::http_port)
      )
      ->config_work();

  return true;
}
void work_facet::add_program_options() {}
}  // namespace doodle