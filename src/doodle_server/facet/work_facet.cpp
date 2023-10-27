//
// Created by td_main on 2023/8/21.
//

#include "work_facet.h"

#include <doodle_core/core/app_base.h>
#include <doodle_core/core/doodle_lib.h>
#include <doodle_core/core/program_info.h>
#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/lib_warp/boost_locale_warp.h>

#include <doodle_app/app/program_options.h>

#include <boost/asio/executor_work_guard.hpp>

#include "core/http_listener.h"
#include <doodle_server/render_farm/detail/url_webscoket.h>
#include <doodle_server/render_farm/work.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <winreg/WinReg.hpp>
namespace doodle {
std::string work_facet::get_server_address() {
  winreg::RegKey l_key{};
  l_key.Open(HKEY_CURRENT_USER, L"Software\\Doodle\\RenderFarm", KEY_READ);
  return conv::utf_to_utf<char>(l_key.GetStringValue(L"server_address"));
}

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
  render_farm::detail::reg_work_websocket{}();
  guard_ = std::make_shared<decltype(guard_)::element_type>(boost::asio::make_work_guard(g_io_context()));

  g_ctx().emplace<doodle::render_farm::work_ptr>(std::make_shared<render_farm::work>())->run(get_server_address());

  return true;
}
void work_facet::add_program_options() {}
}  // namespace doodle