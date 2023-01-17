//
// Created by TD on 2022/9/29.
//

#include "app_command.h"

#include <doodle_core/core/app_facet.h>
#include <doodle_core/core/core_set.h>
#include <doodle_core/core/doodle_lib.h>
#include <doodle_core/core/init_register.h>
#include <doodle_core/core/program_info.h>
#include <doodle_core/gui_template/show_windows.h>

#include <doodle_app/app/facet/gui_facet.h>
#include <doodle_app/app/program_options.h>
#include <doodle_app/gui/main_proc_handle.h>

#include <boost/contract.hpp>
#include <boost/locale.hpp>
namespace doodle {

app_command_base::app_command_base() : app_base(), cmd_str() {
  program_options::emplace();
  cmd_str = win::get_command_line();
  DOODLE_LOG_INFO("获取到命令行 {}", cmd_str);
}

void app_command_base::post_constructor() {
  auto& l_opt = program_options::value();

  for (auto&& [key, val] : facet_list) {
    val->add_program_options();
  }

  for (auto&& [key, val] : l_opt.facet_model) {
    if (val) {
      DOODLE_LOG_INFO("开始运行 {} facet", key);
      run_facet = facet_list[key];
      g_reg()->ctx().emplace<app_facet_ptr>(facet_list[key]);
      break;
    }
  }

  if (!g_reg()->ctx().contains<doodle::app_facet_ptr>()) {
    DOODLE_LOG_INFO("运行默认构面 {}", run_facet->name());
    g_reg()->ctx().emplace<app_facet_ptr>(run_facet);
  }
  boost::asio::post(g_io_context(), [l_f = run_facet]() { (*l_f)(); });
}

bool app_command_base::chick_authorization() {
  DOODLE_LOG_INFO("开始检查授权");
  return chick_build_time();
}

app_command_base& app_command_base::Get() { return *(dynamic_cast<app_command_base*>(self)); }

std::optional<FSys::path> app_command_base::find_authorization_file() const {
  auto l_p = core_set::get_set().program_location() / doodle_config::token_name.data();
  if (!exists(l_p)) {
    l_p = core_set::get_set().get_doc() / doodle_config::token_name.data();
  }
  return exists(l_p) ? std::optional<FSys::path>{l_p} : std::optional<FSys::path>{};
}
bool app_command_base::chick_build_time() const {
  chrono::sys_seconds l_build_time_;
  std::istringstream l_time{version::build_info::get().build_time};
  l_time >> chrono::parse("%Y %m %d %H %M %S", l_build_time_);
  chrono::sys_time_pos l_point{l_build_time_};
  l_point += chrono::months{3};
  return chrono::system_clock::now() < l_point;
}

}  // namespace doodle

namespace doodle {
doodle_main_app::doodle_main_app() : app_command_base() {
  g_reg()->ctx().emplace<gui::main_proc_handle>();
  g_reg()->ctx().emplace<gui::detail::layout_tick>();
}
doodle_main_app& doodle_main_app::Get() { return *(dynamic_cast<doodle_main_app*>(self)); }
bool doodle_main_app::chick_authorization() { return app_command_base::chick_authorization(); }

doodle_main_app::~doodle_main_app() = default;

}  // namespace doodle
