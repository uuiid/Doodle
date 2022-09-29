//
// Created by TD on 2022/9/29.
//

#include "app_command.h"
#include <doodle_core/core/core_set.h>
#include <doodle_core/thread_pool/process_pool.h>
#include <doodle_core/core/doodle_lib.h>
#include <doodle_core/thread_pool/thread_pool.h>

#include <doodle_core/core/init_register.h>
#include <doodle_app/app/program_options.h>

#include <boost/contract.hpp>
#include <boost/locale.hpp>
namespace doodle {
void app_command_base::post_constructor() {
  auto l_opt = g_reg()->ctx().at<program_options_ptr>();
  if (std::holds_alternative<win::string_type>(cmd_str)) {
    l_opt->command_line_parser(
        boost::program_options::split_winmain(boost::locale::conv::utf_to_utf<char>(std::get<win::string_type>(cmd_str)))
    );
  } else if (std::holds_alternative<std::vector<std::string>>(cmd_str)) {
    l_opt->command_line_parser(std::get<std::vector<std::string>>(cmd_str));
  }

  if (!chick_authorization())
    stop_app();

  auto& set = core_set::get_set();
  DOODLE_LOG_INFO("初始化gui日志");
  logger_ctrl::get_log().set_log_name(fmt::format("doodle_{}.txt", fmt::ptr(GetModuleHandleW(nullptr))));
  auto& l_set = core_set::get_set();
  if (l_opt->p_root.first)
    l_set.set_root(l_opt->p_root.second);

  if (l_opt->p_help || l_opt->p_version)
    stop_app();
}

bool app_command_base::chick_authorization() {
  return chick_build_time();
}

void app_command_base::load_back_end() {}

app_command_base& app_command_base::Get() {
  return *(dynamic_cast<app_command_base*>(self));
}
app_command_base::app_command_base(const app_base::in_app_args& in_instance)
    : app_base(in_instance),
      cmd_str(in_instance.in_cmd_line) {
  g_reg()->ctx().at<program_options_ptr>() = std::make_shared<program_options_ptr::element_type>();
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
