//
// Created by TD on 2022/1/18.
//

#include "app_command_base.h"
#include <doodle_core/core/core_set.h>
#include <doodle_core/thread_pool/process_pool.h>
#include <doodle_core/core/doodle_lib.h>
#include <doodle_core/thread_pool/thread_pool.h>

#include <doodle_core/core/init_register.h>
#include <doodle_lib/core/program_options.h>
#include <doodle_lib/core/authorization.h>

#include <boost/contract.hpp>

#include <cryptopp/filters.h>

namespace doodle {
void app_command_base::post_constructor() {
  if (std::holds_alternative<win::string_type>(cmd_str) && std::get<win::string_type>(cmd_str)) {
    options_->command_line_parser(
        boost::program_options::split_winmain(conv::utf_to_utf<char>(std::get<win::string_type>(cmd_str)))
    );
  } else if (std::holds_alternative<std::vector<std::string>>(cmd_str)) {
    options_->command_line_parser(std::get<std::vector<std::string>>(cmd_str));
  }

  if (!chick_authorization())
    stop_app();

  auto& set = core_set::getSet();
  DOODLE_LOG_INFO("初始化gui日志");
  logger_ctrl::get_log().set_log_name(fmt::format("doodle_{}.txt", fmt::ptr(GetModuleHandleW(nullptr))));
  auto& l_set = core_set::getSet();
  if (options_->p_root.first)
    l_set.set_root(options_->p_root.second);
  if (options_->p_max_thread.first) {
    l_set.p_max_thread   = options_->p_max_thread.second;
    p_lib->p_thread_pool = std::make_shared<thread_pool>(l_set.p_max_thread);
  }

  if (options_->p_help || options_->p_version)
    stop_app();
}

bool app_command_base::chick_authorization() {
  auto l_p = core_set::program_location() / doodle_config::token_name.data();
  if (!exists(l_p)) {
    l_p = core_set::getSet().get_doc() / doodle_config::token_name.data();
  }

  return chick_authorization(l_p);
}
bool app_command_base::chick_authorization(const FSys::path& in_path) {
  chrono::sys_seconds l_build_time_;
  std::istringstream l_time{version::build_time};
  l_time >> chrono::parse("%Y %m %d %H %M %S", l_build_time_);
  chrono::sys_time_pos l_point{l_build_time_};
  l_point += chrono::months{3};
  if (chrono::system_clock::now() < l_point)
    return true;

  boost::contract::check l_c =
      boost::contract::public_function(this)
          .precondition([&]() {
            DOODLE_CHICK(!in_path.empty(), doodle_error{"传入路径为空"});
            FSys::is_directory(in_path) ? void() : throw_exception(doodle_error{"传入路径不是文件或者不存在"s});
          });
  if (!exists(in_path)) {
    DOODLE_LOG_ERROR("无法找到授权文件")
    return false;
  }

  FSys::ifstream l_ifstream{in_path};

  std::string ciphertext{std::istreambuf_iterator(l_ifstream), std::istreambuf_iterator<char>()};
  try {
    authorization l_authorization{ciphertext};

    return l_authorization.is_expire();
  } catch (const std::exception& err) {
    DOODLE_LOG_INFO(boost::diagnostic_information(err));
    return false;
  }
}

void app_command_base::load_back_end() {
}

app_command_base& app_command_base::Get() {
  return *(dynamic_cast<app_command_base*>(self));
}
app_command_base::app_command_base(const app_base::in_app_args& in_instance)
    : app_base(in_instance),
      options_(std::make_shared<program_options>()),
      cmd_str(in_instance.in_cmd_line) {
}
}  // namespace doodle
