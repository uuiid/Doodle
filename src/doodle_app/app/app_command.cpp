//
// Created by TD on 2022/9/29.
//

#include "app_command.h"
#include <doodle_core/core/core_set.h>
#include <doodle_core/core/doodle_lib.h>
#include <doodle_core/core/program_info.h>
#include <doodle_core/gui_template/show_windows.h>

#include <doodle_core/thread_pool/process_pool.h>
#include <doodle_core/thread_pool/thread_pool.h>

#include <doodle_core/core/init_register.h>
#include <doodle_core/core/app_facet.h>

#include <doodle_app/gui/main_proc_handle.h>
#include <doodle_app/app/program_options.h>
#include <doodle_app/app/facet/gui_facet.h>

#include <boost/contract.hpp>
#include <boost/locale.hpp>
#include <doodle_app/app/program_options.h>
namespace doodle {

app_command_base::app_command_base(const app_base::in_app_args& in_instance)
    : app_base(in_instance),
      cmd_str(in_instance.in_cmd_line) {
  g_reg()->ctx().emplace<program_options_ptr>(std::make_shared<program_options_ptr::element_type>());
}

app_command_base::app_command_base()
    : app_base(),
      cmd_str(boost::program_options::split_winmain(std::string{
          boost::locale::conv::utf_to_utf<char>(GetCommandLineW())})) {
  DOODLE_LOG_INFO("获取到命令行 {}", std::string{boost::locale::conv::utf_to_utf<char>(GetCommandLineW())});
  g_reg()->ctx().emplace<program_options_ptr>(std::make_shared<program_options_ptr::element_type>());
}

void app_command_base::post_constructor() {
  auto l_opt = g_reg()->ctx().at<program_options_ptr>();

  for (auto&& [key, val] : facet_list) {
    val->add_program_options(l_opt);
    l_opt->build_opt(key);
  }

  if (std::holds_alternative<win::string_type>(cmd_str)) {
    l_opt->command_line_parser(
        boost::program_options::split_winmain(boost::locale::conv::utf_to_utf<char>(std::get<win::string_type>(cmd_str)))
    );
  } else if (std::holds_alternative<std::vector<std::string>>(cmd_str)) {
    l_opt->command_line_parser(std::get<std::vector<std::string>>(cmd_str));
  }

  for (auto&& [key, val] : l_opt->facet_model) {
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
  boost::asio::post(g_io_context(), [l_f = run_facet]() {
    (*l_f)();
  });
}

bool app_command_base::chick_authorization() {
  return chick_build_time();
}

app_command_base& app_command_base::Get() {
  return *(dynamic_cast<app_command_base*>(self));
}

std::optional<FSys::path> app_command_base::find_authorization_file() const {
  auto l_p = core_set::program_location() / doodle_config::token_name.data();
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

doodle_main_app::doodle_main_app(const in_gui_arg& in_arg)
    : app_command_base(in_arg) {
  g_reg()->ctx().emplace<gui::main_proc_handle>();
  g_reg()->ctx().emplace<gui::detail::layout_tick>();
  auto& l_p = g_reg()->ctx().at<program_info>();
  l_p.parent_windows_attr(in_arg.in_parent);
}
doodle_main_app::doodle_main_app()
    : app_command_base() {
}
doodle_main_app& doodle_main_app::Get() {
  return *(dynamic_cast<doodle_main_app*>(self));
}
bool doodle_main_app::chick_authorization() {
  return app_command_base::chick_authorization();
}

doodle_main_app::~doodle_main_app() = default;

}  // namespace doodle
