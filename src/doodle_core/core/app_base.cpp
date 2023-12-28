//
// Created by TD on 2022/5/30.
//

#include "app_base.h"

#include <doodle_core/core/app_facet.h>
#include <doodle_core/core/core_set.h>
#include <doodle_core/core/doodle_lib.h>
#include <doodle_core/core/init_register.h>
#include <doodle_core/core/program_info.h>
#include <doodle_core/database_task/sqlite_client.h>
#include <doodle_core/gui_template/show_windows.h>
#include <doodle_core/logger/logger.h>

#include <boost/asio.hpp>
#include <boost/locale.hpp>

#include "core/app_base.h"
#include <memory>
#include <thread>

namespace doodle {

void app_base::cancellation_signals::emit(boost::asio::cancellation_type ct) {
  std::lock_guard<std::mutex> _(mtx);

  for (auto& sig : sigs) sig.emit(ct);
}

boost::asio::cancellation_slot app_base::cancellation_signals::slot() {
  std::lock_guard<std::mutex> _(mtx);

  auto itr = std::find_if(sigs.begin(), sigs.end(), [](boost::asio::cancellation_signal& sig) {
    return !sig.slot().has_handler();
  });

  if (itr != sigs.end())
    return itr->slot();
  else
    return sigs.emplace_back().slot();
}

app_base* app_base::self = nullptr;

app_base::app_base(int argc, const char* const argv[])
    : p_title(boost::locale::conv::utf_to_utf<wchar_t>(fmt::format("doodle {}", version::build_info::get().version_str))
      ),
      stop_(false),
      lib_ptr(std::make_shared<doodle_lib>()),
      arg_{argc, argv} {
  self                   = this;
  auto&& l_program_info  = g_ctx().emplace<program_info>();
  l_program_info.handle_ = ::GetModuleHandleW(nullptr);
  init();
}

void app_base::init() {
  DOODLE_LOG_INFO("开始初始化基本配置");

  core_set_init k_init{};

  DOODLE_LOG_INFO("寻找用户配置文件目录");
  k_init.config_to_user();
  DOODLE_LOG_INFO("读取配置文件");
  k_init.read_file();
  DOODLE_LOG_INFO("寻找到自身exe {}", core_set::get_set().program_location());
  boost::asio::post(g_io_context(), [this]() { this->post_constructor(); });
}
app_base::~app_base() = default;

app_base& app_base::Get() { return *self; }
std::int32_t app_base::run() {
  if (stop_) return 0;
  g_io_context().run();
  return 0;
}

std::int32_t app_base::poll_one() {
  if (stop_) return 0;
  g_io_context().poll_one();
  return 0;
}
void app_base::stop_app(bool in_stop) {
  on_cancel.emit();
  on_stop();
  g_ctx().emplace<program_info>().is_stop = true;
  this->deconstruction();
  core_set_init{}.write_file();
}

bool app_base::is_main_thread() const { return run_id == std::this_thread::get_id(); }

}  // namespace doodle
