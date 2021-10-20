#include "logger.h"

#include <doodle_lib/core/core_set.h>
#include <doodle_lib/doodle_lib_fwd.h>
#include <spdlog/async.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/rotating_file_sink.h>

namespace doodle {
namespace details {
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
            .c_str());
#else
    std::cout << fmt::to_string(formatted) << std::endl;
#endif
  }

  void flush_() override {}
};
}  // namespace details
using msvc_doodle_sink_mt       = details::msvc_doodle_sink<std::mutex>;

logger_ctrl *logger_ctrl::_self = nullptr;

logger_ctrl::logger_ctrl()
    : p_log_path(core_set::getSet().get_cache_root() / "log"),
      p_log_name("tmp_logfile" + date::format("_%y_%m_%d_%H_%M_%S_", std::chrono::system_clock::now()) + ".txt") {
  init_temp_log();
}
void logger_ctrl::init_temp_log() {
  init_log();

  auto l_path = p_log_path / p_log_name;
  try {
    using namespace std::chrono_literals;
    spdlog::init_thread_pool(8192, 1);
    auto l_file   = new_object<spdlog::sinks::rotating_file_sink_mt>(l_path.generic_string(), 1024 * 1024, 100, true);
    auto l_logger = new_object<spdlog::async_logger>(
        "doodle_lib", l_file, spdlog::thread_pool(), spdlog::async_overflow_policy::block);
#if !defined(NDEBUG)
    auto l_k_debug = new_object<msvc_doodle_sink_mt>();
    l_logger->sinks().push_back(l_k_debug);
#endif

    spdlog::register_logger(l_logger);
    spdlog::set_default_logger(l_logger);

    spdlog::flush_every(3s);
    spdlog::set_level(spdlog::level::debug);
  } catch (const spdlog::spdlog_ex &spdlog_ex) {
    std::cout << "日志初始化失败" << spdlog_ex.what() << std::endl;
  }
  SPDLOG_DEBUG(fmt::format("初始化gebug日志 {}", "ok"));
  SPDLOG_INFO(fmt::format("初始化信息日志 {}", "ok"));
  SPDLOG_WARN(fmt::format("初始化警告日志 {}", "ok"));
  SPDLOG_ERROR(fmt::format("初始化错误日志 {}", "ok"));
}
void logger_ctrl::init_log() {
  p_log_path = core_set::getSet().get_cache_root() / "log";
  if (!FSys::exists(p_log_path))
    FSys::create_directories(p_log_path);
}
logger_ctrl &logger_ctrl::get_log() {
  return *_self;
}
bool logger_ctrl::set_log_name(const std::string &in_name) {
  init_log();
  p_log_name  = in_name;

  auto l_log  = spdlog::get("doodle_lib");
  auto &l_v   = l_log->sinks();
  auto l_path = p_log_path / p_log_name;
  auto l_file = new_object<spdlog::sinks::rotating_file_sink_mt>(l_path.generic_string(), 1024 * 1024, 100, true);
  l_v.push_back(l_file);
  /// 去除掉临时的的文件记录器
  l_v.erase(l_v.begin());
  return true;
}
void logger_ctrl::post_constructor() {
  _self = this;
}
logger_ctrl::~logger_ctrl() {
  spdlog::shutdown();
}
}  // namespace doodle
