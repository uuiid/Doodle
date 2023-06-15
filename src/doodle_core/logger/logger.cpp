#include "logger.h"

#include <doodle_core/core/core_set.h>

#include <boost/locale.hpp>
#include <boost/process/environment.hpp>

#include <Windows.h>
#include <chrono>
#include <date/date.h>
#include <date/tz.h>
#include <fmt/core.h>
#include <spdlog/async.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/daily_file_sink.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_sinks.h>

namespace doodle::details {
template <class Mutex>
class msvc_doodle_sink : public spdlog::sinks::base_sink<Mutex> {
 public:
  msvc_doodle_sink() = default;

 protected:
  void sink_it_(const spdlog::details::log_msg &msg) override {
    spdlog::memory_buf_t formatted;
    spdlog::sinks::base_sink<Mutex>::formatter_->format(msg, formatted);
#ifdef _WIN32
    OutputDebugString(
#ifdef UNICODE
        boost::locale::conv::utf_to_utf<wchar_t>(
#endif
            fmt::to_string(formatted)
#ifdef UNICODE
        )
#endif
            .c_str()
    );
#else
    std::cout << fmt::to_string(formatted) << std::endl;
#endif
  }

  void flush_() override {}
};
using msvc_doodle_sink_mt = msvc_doodle_sink<std::mutex>;

logger_ctrl::logger_ctrl() : p_log_path(FSys::temp_directory_path() / "doodle" / "log") {
  spdlog::init_thread_pool(8192, 1);
  init_temp_log();
}

auto make_log(const FSys::path &in_path, const std::string &in_name = "doodle_lib"s) {
  if (!FSys::exists(in_path)) FSys::create_directories(in_path);
  auto l_path = in_path / fmt::format(
                              "{:%Y-%m-%d %H-%M-%S}_{}_{}.txt", chrono::system_clock::now(),
                              boost::this_process::get_id(), in_name
                          );
  std::shared_ptr<spdlog::async_logger> l_logger;
  try {
    auto l_file =
        std::make_shared<spdlog::sinks::rotating_file_sink_mt>(l_path.generic_string(), 1024 * 1024 * 1024, 100, true);
    l_logger = std::make_shared<spdlog::async_logger>(
        in_name, l_file, spdlog::thread_pool(), spdlog::async_overflow_policy::block
    );
#if !defined(NDEBUG)
    auto l_k_debug = std::make_shared<msvc_doodle_sink_mt>();
    l_logger->sinks().push_back(l_k_debug);
    // auto l_stdout_sink_mt = std::make_shared<spdlog::sinks::stdout_sink_mt>();
    // l_logger->sinks().push_back(l_stdout_sink_mt);
#endif
    spdlog::register_logger(l_logger);

  } catch (const spdlog::spdlog_ex &spdlog_ex) {
    std::cout << "日志初始化失败" << boost::diagnostic_information(spdlog_ex) << std::endl;
  }

  return l_logger;
}

void logger_ctrl::init_temp_log() {
  auto l_logger = make_log(p_log_path);
  spdlog::set_default_logger(l_logger);

  spdlog::flush_every(3s);
  spdlog::set_level(spdlog::level::debug);
}

logger_ctrl::~logger_ctrl() {
  spdlog::apply_all([](const std::shared_ptr<spdlog::logger> &in_ptr) { in_ptr->flush(); });
  spdlog::shutdown();
}
bool logger_ctrl::add_log_sink(const std::shared_ptr<spdlog::sinks::sink> &in_ptr, const std::string &in_name) {
  auto l_logger = make_log(p_log_path, in_name);
  l_logger->sinks().emplace_back(in_ptr);
  auto l_name = spdlog::default_logger()->name();
  spdlog::set_default_logger(l_logger);
  /// 刷新所有
  spdlog::apply_all([](const std::shared_ptr<spdlog::logger> &in_ptr) { in_ptr->flush(); });
  /// 去除旧的log
  spdlog::drop(l_name);

  SPDLOG_DEBUG(fmt::format("初始化 {} 日志 {}", in_name));
  return true;
}
}  // namespace doodle::details
