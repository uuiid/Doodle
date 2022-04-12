//
// Created by TD on 2022/1/18.
//

#include "app_base.h"
#include <doodle_lib/core/core_set.h>
#include <doodle_lib/core/init_register.h>
#include <doodle_lib/long_task/process_pool.h>
#include <doodle_lib/core/doodle_lib.h>
#include <doodle_lib/core/program_options.h>
#include <doodle_lib/thread_pool/thread_pool.h>
#include <doodle_lib/core/authorization.h>

#include <boost/contract.hpp>

#include <cryptopp/modes.h>
#include <cryptopp/aes.h>
#include <cryptopp/gcm.h>
#include <cryptopp/filters.h>

namespace doodle {
app_base* app_base::self = nullptr;
app_base::app_base()
    : p_title(conv::utf_to_utf<wchar_t>(fmt::format(
          "doodle {}", version::version_str))),
      stop_(false),
      instance(::GetModuleHandleW(nullptr)),
      p_lib(std::make_shared<doodle_lib>()),
      options_(std::make_shared<program_options>()) {
  self = this;

  DOODLE_LOG_INFO("开始初始化基本配置");

  core_set_init k_init{};

  DOODLE_LOG_INFO("寻找缓存目录");
  k_init.find_cache_dir();
  p_lib->create_time_database();
  DOODLE_LOG_INFO("寻找用户配置文件目录");
  k_init.config_to_user();
  k_init.find_maya();
  DOODLE_LOG_INFO("读取配置文件");
  k_init.read_file();
  g_bounded_pool().timiter_ = core_set::getSet().p_max_thread;
  g_main_loop().attach<one_process_t>([this]() {
    init_register::instance().reg_class();
    this->load_back_end();
  });
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
  while (!is_stop()) {
    loop_one();
  }
  return 0;
}

void app_base::command_line_parser(const std::vector<string>& in_arg) {
  if (!chick_authorization())
    stop_app();

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
    stop_app();
}

void app_base::command_line_parser(const LPSTR& in_arg) {
  auto k_str = boost::program_options::split_winmain(in_arg);
  return command_line_parser(k_str);
}
bool app_base::chick_authorization(const FSys::path& in_path) {
  boost::contract::check l_c =
      boost::contract::public_function(this)
          .precondition([&]() {
            chick_true<doodle_error>(!in_path.empty(), DOODLE_LOC, "传入路径为空");
            chick_true<doodle_error>(!FSys::is_directory(in_path),
                                     DOODLE_LOC,
                                     "传入路径不是文件或者不存在");
          });
  FSys::ifstream l_ifstream{in_path};
  std::string ciphertext{std::istreambuf_iterator(l_ifstream), std::istreambuf_iterator<char>()};
  authorization l_authorization{ciphertext};

  return l_authorization.is_expire();
}
bool app_base::chick_authorization() {
  auto l_p = core_set::getSet().get_doc() / doodle_config::token_name;
  if (!exists(l_p)) {
    l_p = FSys::current_path() / doodle_config::token_name;
  }
  if (!exists(l_p)) {
    DOODLE_LOG_ERROR("无法找到授权文件")
    return false;
  }

  return chick_authorization(l_p);
}
void app_base::stop_app(bool in_stop) {
  g_main_loop().abort(in_stop);
  g_bounded_pool().abort(in_stop);
  if (!stop_)
    g_main_loop().attach<one_process_t>([this]() {
      core_set_init{}.write_file();
    });
  this->stop_ = true;
}
void app_base::post_quit_message() {
  ::PostQuitMessage(0);
}
bool app_base::is_stop() const {
  return g_main_loop().empty() &&
         g_bounded_pool().empty() &&
         stop_;
}

app_base::~app_base() = default;

void app_command_base::load_back_end() {
}

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
