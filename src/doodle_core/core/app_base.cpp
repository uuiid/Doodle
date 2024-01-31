//
// Created by TD on 2022/5/30.
//

#include "app_base.h"

#include <doodle_core/core/app_facet.h>
#include <doodle_core/core/core_set.h>
#include <doodle_core/core/doodle_lib.h>
#include <doodle_core/core/program_info.h>
#include <doodle_core/database_task/sqlite_client.h>
#include <doodle_core/gui_template/show_windows.h>
#include <doodle_core/logger/logger.h>

#include <boost/asio.hpp>
#include <boost/locale.hpp>

#include "core/app_base.h"
#include <memory>
#include <thread>
#include <wil/result.h>

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
  default_logger_raw()->log(log_loc(), level::warn, "开始初始化基本配置");
  default_logger_raw()->flush();
}

namespace {
struct comline_data {
  explicit comline_data(std::int32_t argc, const wchar_t* const argv[]) {
    for (std::int32_t i = 0; i < argc; ++i) {
      arg_data.emplace_back(boost::locale::conv::utf_to_utf<char>(argv[i]));
    }
    p_buff = std::make_shared<char*[]>(argc);
    for (std::int32_t i = 0; i < argc; ++i) {
      p_buff[i] = arg_data[i].data();
    }
    argv_ = p_buff.get();
  }
  std::vector<std::string> arg_data;
  std::shared_ptr<char*[]> p_buff;
  const char* const* argv_;
};

}  // namespace

app_base::app_base(std::int32_t argc, const wchar_t* const argv[]) : app_base(argc, comline_data{argc, argv}.argv_) {}

app_base::~app_base() = default;

app_base& app_base::Get() { return *self; }
std::int32_t app_base::run() {
  if (stop_) return 0;
  try {
    g_io_context().run();
  } catch (...) {
    default_logger_raw()->log(log_loc(), level::err, boost::current_exception_diagnostic_information());
  }
  return 0;
}

std::int32_t app_base::poll_one() {
  if (stop_) return 0;
  try {
    g_io_context().poll_one();
  } catch (...) {
    default_logger_raw()->log(log_loc(), level::err, boost::current_exception_diagnostic_information());
  }
  return 0;
}
void app_base::stop_app(bool in_stop) {
  on_cancel.emit();
  on_stop();
  g_ctx().emplace<program_info>().is_stop = true;
  facets_.clear();
  core_set_init{}.write_file();
}

bool app_base::is_main_thread() const { return run_id == std::this_thread::get_id(); }
void app_base::write_current_error_tmp_dir() {
  auto l_tmp = FSys::temp_directory_path() / "doodle" / "log";
  if (!FSys::exists(l_tmp)) FSys::create_directories(l_tmp);
  auto l_path = l_tmp / fmt::format("doodle_error_{}.txt", boost::this_process::get_id());
  FSys::ofstream l_ofstream{l_path};
  l_ofstream << boost::current_exception_diagnostic_information() << std::endl;
}
}  // namespace doodle
