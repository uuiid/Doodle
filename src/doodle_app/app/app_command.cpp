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
namespace details::app_command_base {
void run_facet(const app_base::app_facet_map& in_map, app_base::app_facet_ptr& in_def_facet) {
  auto& l_opt = doodle::program_options::value();

  for (auto&& [key, val] : in_map) {
    val->add_program_options();
  }
  l_opt.command_line_parser();

  for (auto&& [key, val] : l_opt.facet_model) {
    if (val) {
      DOODLE_LOG_INFO("开始运行 {} facet", key);
      in_def_facet = in_map.at(key);
      g_reg()->ctx().emplace<app_facet_ptr>(in_map.at(key));
      break;
    }
  }

  if (!g_reg()->ctx().contains<doodle::app_facet_ptr>()) {
    DOODLE_LOG_INFO("运行默认构面 {}", in_def_facet->name());
    g_reg()->ctx().emplace<app_facet_ptr>(in_def_facet);
  }
  boost::asio::post(g_io_context(), [l_f = in_def_facet]() { (*l_f)(); });
}

}  // namespace details::app_command_base

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
