//
// Created by TD on 2022/5/30.
//

#include "app_base.h"

#include <doodle_core/core/core_set.h>
#include <doodle_core/core/doodle_lib.h>
#include <doodle_core/logger/logger.h>

#include <boost/asio.hpp>
#include <boost/locale.hpp>
#include <boost/system/detail/error_code.hpp>

#include "core/global_function.h"
#include <memory>
#include <processthreadsapi.h>
#include <processtopologyapi.h>
#include <spdlog/spdlog.h>
#include <thread>
#include <wil/result.h>
#include <wil/result_macros.h>
#include <winnt.h>

namespace doodle {

app_base* app_base::self = nullptr;
bool app_base::init() { return true; }
app_base::app_base(int argc, const char* const argv[])
    : stop_(false), lib_ptr(std::make_shared<doodle_lib>()), arg_{argc, argv} {
  self = this;
  default_logger_raw()->log(log_loc(), level::warn, "开始初始化基本配置");
  default_logger_raw()->flush();
  add_signal();
}
void app_base::set_exit_code(std::int32_t in_exit_code) {
  if (exit_code == 0) exit_code = in_exit_code;
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
app_base* app_base::GetPtr() { return self; }

void app_base::bind_thread_to_group(int group_id) {
  // 获取组中处理器数量
  auto l_a = GetActiveProcessorCount(group_id);

  wil::unique_handle process_handle{GetCurrentProcess()};
  GROUP_AFFINITY group_affinity{};
  group_affinity.Group = static_cast<WORD>(group_id);
  // 设置掩码, 选择组内的所有处理器
  for (decltype(l_a) i = 0; i < l_a; ++i) {
    group_affinity.Mask |= (1ULL << i);
  }
  LOG_IF_WIN32_BOOL_FALSE(::SetThreadGroupAffinity(GetCurrentThread(), &group_affinity, nullptr));
}

std::int32_t app_base::run() {
  stop_ = !init();
  if (stop_) return 0;
  auto l_logger = g_logger_ctrl().get_main_error();
  if (!use_multithread_) {
    try {
      g_io_context().run();
    } catch (const doodle_error& in_err) {
      set_exit_code(in_err.error_code_ == 0 ? -1 : in_err.error_code_);
      l_logger->error(in_err.what());
    } catch (const std::system_error& in_err) {
      set_exit_code(in_err.code().value());
      l_logger->error(in_err.what());
    } catch (...) {
      set_exit_code(1);
      l_logger->error(boost::current_exception_diagnostic_information());
    }
  } else {
    std::vector<std::thread> l_threads{};
    auto l_thread_count = get_hardware_concurrency() == 0 ? 8 : get_hardware_concurrency() - 1;
    l_threads.reserve(l_thread_count);
    for (std::size_t i = 0; i < l_thread_count; ++i) {
      l_threads.emplace_back([this, i, l_logger]() {
        bind_thread_to_group(static_cast<int>(i % static_cast<std::size_t>(GetActiveProcessorGroupCount())));
        for (;;) try {
            g_io_context().run();
            break;
          } catch (...) {
            l_logger->error(boost::current_exception_diagnostic_information());
          }
      });
    }
    for (auto& l_thread : l_threads) {
      l_thread.join();
    }
  }

  facets_.clear();
  spdlog::apply_all([](const std::shared_ptr<spdlog::logger>& in_ptr) { in_ptr->flush(); });
  return exit_code;
}

void app_base::stop_app(std::int32_t in_exit_code) {
  stop_     = true;
  exit_code = in_exit_code;
  on_cancel.emit();
  facets_.clear();
  spdlog::apply_all([](const std::shared_ptr<spdlog::logger>& in_ptr) { in_ptr->flush(); });
  g_io_context().stop();
}

bool app_base::is_main_thread() const { return run_id == std::this_thread::get_id(); }
void app_base::use_multithread(bool in_use) { use_multithread_ = in_use; }

void app_base::add_signal() {
  sig_ptr = std::make_shared<signal_t>(g_io_context(), SIGINT, SIGTERM);
  sig_ptr->async_wait([this](const boost::system::error_code& ec, int signal_number) {
    if (ec) return;
    stop_app(0);
  });
}
}  // namespace doodle
