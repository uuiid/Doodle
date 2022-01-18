//
// Created by TD on 2022/1/18.
//

#include "app_base.h"
#include <doodle_lib/core/core_set.h>
#include <doodle_lib/long_task/process_pool.h>
#include <doodle_lib/core/doodle_lib.h>
#include <doodle_lib/core/program_options.h>
#include <doodle_lib/thread_pool/thread_pool.h>
namespace doodle {
app_base* app_base::self = nullptr;
app_base::app_base()
    : p_title(conv::utf_to_utf<wchar_t>(fmt::format(
          "doodle {}.{}.{}.{}",
          Doodle_VERSION_MAJOR,
          Doodle_VERSION_MINOR,
          Doodle_VERSION_PATCH,
          Doodle_VERSION_TWEAK))),
      stop_(true),
      instance(::GetModuleHandleW(nullptr)),
      p_lib(std::make_shared<doodle_lib>()),
      options_() {
  self = this;

  DOODLE_LOG_INFO("开始初始化基本配置");

  core_set_init k_init{};

  DOODLE_LOG_INFO("寻找缓存目录");
  k_init.find_cache_dir();
  DOODLE_LOG_INFO("寻找用户配置文件目录");
  k_init.config_to_user();
  k_init.find_maya();
  DOODLE_LOG_INFO("读取配置文件");
  k_init.read_file();
  g_bounded_pool().set_bounded(boost::numeric_cast<std::uint16_t>(core_set::getSet().p_max_thread));
}
app_base::app_base(win::wnd_instance const& in_instance)
    : app_base() {
  instance = in_instance;
  self     = this;
}
std::atomic_bool& app_base::stop() {
  stop_ = true;
  return stop_;
}
bool app_base::valid() const {
  return false;
}
app_base& app_base::Get() {
  return *self;
}
std::int32_t app_base::run() {
  while (!stop_) {
    loop_one();
  }
  return 0;
}
void app_base::command_line_parser(const std::vector<string>& in_arg) {
  options_->command_line_parser(in_arg);

  auto& set = core_set::getSet();
  DOODLE_LOG_INFO("初始化gui日志");
  logger_ctrl::get_log().set_log_name("doodle_gui.txt");
  auto& l_set = core_set::getSet();
  if (options_->p_root.first)
    l_set.set_root(options_->p_root.second);
  if (options_->p_max_thread.first) {
    l_set.p_max_thread   = options_->p_max_thread.second;
    p_lib->p_thread_pool = std::make_shared<thread_pool>(l_set.p_max_thread);
  }

  if (options_->p_help || options_->p_version)
    stop_ = true;
}

void app_base::command_line_parser(const LPSTR& in_arg) {
  auto k_str = boost::program_options::split_winmain(in_arg);
  return command_line_parser(k_str);
}
app_base::~app_base() = default;

void app_command_base::loop_one() {
  static decltype(chrono::system_clock::now()) s_now{chrono::system_clock::now()};
  decltype(chrono::system_clock::now()) l_now{chrono::system_clock::now()};
  g_main_loop().update(l_now - s_now, nullptr);
  g_bounded_pool().update(l_now - s_now, nullptr);
  s_now = l_now;
}
app_command_base& app_command_base::Get() {
  return *(dynamic_cast<app_command_base*>(self));
}
}  // namespace doodle
