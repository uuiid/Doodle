//
// Created by TD on 2022/5/30.
//

#include "app_base.h"
#include <doodle_core/core/core_set.h>
#include <doodle_core/core/program_info.h>
#include <doodle_core/gui_template/show_windows.h>
#include <doodle_core/thread_pool/process_pool.h>
#include <doodle_core/core/doodle_lib.h>
#include <doodle_core/thread_pool/thread_pool.h>
#include <doodle_core/core/init_register.h>
#include <doodle_core/logger/logger.h>
#include <doodle_core/database_task/sqlite_client.h>

#include <boost/locale.hpp>
#include <boost/asio.hpp>

#include <doodle_core/core/app_facet.h>
namespace doodle {

app_base* app_base::self = nullptr;

app_base::app_base()
    : p_title(boost::locale::conv::utf_to_utf<wchar_t>(fmt::format(
          "doodle {}", version::build_info::get().version_str
      ))),
      stop_(false),
      p_lib(std::make_shared<doodle_lib>()) {
  self = this;
  g_reg()->ctx().at<program_info>().handle_ = ::GetModuleHandleW(nullptr);
  boost::asio::post(g_io_context(), [this]() {
    this->init();
  });
}

app_base::app_base(const app_base::in_app_args& in_arg)
    : app_base() {
  g_reg()->ctx().at<program_info>().handle_ = in_arg.in_instance;
}
void app_base::init() {
  DOODLE_LOG_INFO("开始初始化基本配置");

  core_set_init k_init{};

  p_lib->create_time_database();
  DOODLE_LOG_INFO("寻找用户配置文件目录");
  k_init.config_to_user();
  k_init.find_maya();
  DOODLE_LOG_INFO("读取配置文件");
  k_init.read_file();
  g_bounded_pool().timiter_ = core_set::get_set().p_max_thread;
  this->post_constructor();

  boost::asio::post(g_io_context(), []() {
    init_register::instance().reg_class();
  });
}
app_base::~app_base() = default;

std::atomic_bool& app_base::stop() {
  stop_ = true;
  return stop_;
}
app_base& app_base::Get() {
  return *self;
}
std::int32_t app_base::run() {
  g_io_context().run();
  clear_loop();
  return 0;
}

std::int32_t app_base::poll_one() {
  g_io_context().poll_one();
  return 0;
}
void app_base::stop_app(bool in_stop) {
  g_bounded_pool().abort(in_stop);
  g_reg()->clear<gui::detail::windows_tick, gui::detail::windows_render>();
  core_set_init{}.write_file();

  g_reg()->ctx().at<program_info>().is_stop = true;
  this->stop_                               = true;
}

void app_base::load_project(const FSys::path& in_path) const {
  if (!in_path.empty() &&
      FSys::exists(in_path) &&
      FSys::is_regular_file(in_path) &&
      in_path.extension() == doodle_config::doodle_db_name.data()) {
    g_reg()->ctx().at<database_n::file_translator_ptr>()->async_open(
        in_path, [](bsys::error_code) -> void {
          DOODLE_LOG_INFO("完成打开项目");
        }
    );
  }
}
void app_base::clear_loop() {
  while (!g_bounded_pool().empty()) {
    static decltype(chrono::system_clock::now()) s_now{chrono::system_clock::now()};
    decltype(chrono::system_clock::now()) l_now{chrono::system_clock::now()};
    g_bounded_pool().update(l_now - s_now, nullptr);
    s_now = l_now;
  }
}

void app_base::add_facet(const app_facet_ptr& in_facet) {
  facet_list.emplace(in_facet->name(), in_facet);
}

}  // namespace doodle
