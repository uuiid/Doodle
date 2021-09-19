#include <DoodleLib/DoodleLib_fwd.h>
#include <Logger/Logger.h>
#include <core/CoreSet.h>
#include <spdlog/async.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/msvc_sink.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/spdlog.h>

#ifdef _WIN32
#include <windows.h>

#endif

#include <boost/locale.hpp>
namespace doodle::Logger {

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

using msvc_doodle_sink_mt = msvc_doodle_sink<std::mutex>;
// using msvc_doodle_sink_st = msvc_doodle_sink<spdlog::details::null_mutex>;

static void boostLoggerInitAsyn(const std::string &logPath,
                                std::size_t logMaxSize) {
  auto appdata = CoreSet::getSet().getCacheRoot();
  appdata /= logPath;
  appdata /= "log";
  if (!FSys::exists(appdata)) {
    FSys::create_directories(appdata);
  }
  appdata /= "logfile_" + date::format("_%y_%m_%d_%H_%M_%S_", std::chrono::system_clock::now()) + ".txt";

  try {
    using namespace std::chrono_literals;
    spdlog::init_thread_pool(8192, 1);
#ifdef NDEBUG
    auto k_file = new_object<spdlog::sinks::rotating_file_sink_mt>(appdata.generic_string(), 1024 * 1024, 100);
    std::vector<spdlog::sink_ptr> sinks{k_file};
#else
    auto k_debug = new_object<msvc_doodle_sink_mt>();

    auto k_file = new_object<spdlog::sinks::rotating_file_sink_mt>(appdata.generic_string(), 1024 * 1024, 100, true);
    std::vector<spdlog::sink_ptr> sinks{k_file, k_debug};
#endif  // NDEBUG
    auto k_logger = new_object<spdlog::async_logger>(
        "doodle_lib", sinks.begin(), sinks.end(), spdlog::thread_pool(), spdlog::async_overflow_policy::block);

    spdlog::register_logger(k_logger);

    spdlog::set_default_logger(k_logger);
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

void doodle_initLog(const std::string &logPath, std::size_t logMaxSize, bool async) {
  boostLoggerInitAsyn(logPath, logMaxSize);
}
void DOODLELIB_API clear() {
  spdlog::shutdown();
}
}  // namespace doodle::Logger
