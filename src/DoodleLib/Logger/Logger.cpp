#include <DoodleLib/DoodleLib_fwd.h>
#include <Logger/Logger.h>
#include <Logger/LoggerTemplate.h>

#include <boost/log/sinks.hpp>
//windows头
#include <Shlobj.h>
#include <spdlog/async.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/msvc_sink.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/spdlog.h>
#include <windows.h>

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/dll.hpp>
#include <boost/dll/runtime_symbol_info.hpp>
#include <boost/filesystem.hpp>
#include <boost/locale.hpp>
#include <boost/log/exceptions.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/sinks.hpp>
#include <boost/log/sinks/async_frontend.hpp>
#include <boost/log/sinks/sync_frontend.hpp>
#include <boost/log/support/date_time.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/utility/setup.hpp>
namespace doodle::Logger {
//using file_sink = boost::log::sinks::asynchronous_sink<boost::log::sinks::text_file_backend>;
//using sink_t    = boost::log::sinks::synchronous_sink<boost::log::sinks::debug_output_backend>;
// New macro that includes severity, filename and line number
#define LOG_ADD_FILE boost::log::attribute_cast<boost::log::attributes::mutable_constant<std::string>>(::boost::log::trivial::logger::get().get_attributes()["File"]).set(__FILE__)
#define LOG_ADD_LINE boost::log::attribute_cast<boost::log::attributes::mutable_constant<int>>(::boost::log::trivial::logger::get().get_attributes()["Line"]).set(__LINE__)

// static QString gLogDir;
// static int gLogMaxCount;

// static void outputMessage(QtMsgType type, const QMessageLogContext &context,
//                           const QString &msg);
// static void outputMessageAsync(QtMsgType type,
//                                const QMessageLogContext &context,
//                                const QString &msg);

static void
boostLoggerInitAsyn(const std::string &logPath,
                    std::size_t logMaxSize);

void boostLoggerInitAsyn(const std::string &logPath,
                         std::size_t logMaxSize) {

  auto appdata = boost::filesystem::current_path();
  appdata /= logPath;
  appdata /= "log";
  if (!boost::filesystem::exists(appdata)) {
    boost::filesystem::create_directories(appdata);
  }
  appdata /= "logfile.txt";

  try {
    using namespace std::chrono_literals;
    spdlog::init_thread_pool(8192, 1);
#ifdef NDEBUG
    auto k_file  = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(appdata.generic_string(), 1024 * 1024, 100);
    std::vector<spdlog::sink_ptr> sinks{k_file};
#else
    auto k_debug = std::make_shared<spdlog::sinks::msvc_sink_mt>();
    auto k_file  = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(appdata.generic_string(), 1024 * 1024, 100);
    std::vector<spdlog::sink_ptr> sinks{k_file, k_debug};
#endif  //NDEBUG
    auto k_logger = std::make_shared<spdlog::async_logger>(
        "doodle_lib", sinks.begin(), sinks.end(), spdlog::thread_pool(), spdlog::async_overflow_policy::block);
    spdlog::register_logger(k_logger);

    spdlog::set_default_logger(k_logger);
    spdlog::flush_every(3s);
    spdlog::set_level(spdlog::level::debug);
  } catch (const spdlog::spdlog_ex &spdlog_ex) {
    std::cout << "日志初始化失败" << spdlog_ex.what() << std::endl;
  }

  SPDLOG_DEBUG(fmt::format("初始化gebug日志 {}","ok"));
  SPDLOG_INFO(fmt::format("初始化信息日志 {}","ok"));
  SPDLOG_WARN(fmt::format("初始化警告日志 {}","ok"));
  SPDLOG_ERROR(fmt::format("初始化错误日志 {}","ok"));
  spdlog::source_loc{};
}

void doodle_initLog(const std::string &logPath, std::size_t logMaxSize, bool async) {
  boostLoggerInitAsyn(logPath, logMaxSize);
}
void DOODLELIB_API clear() {
  spdlog::shutdown();
}
}  // namespace doodle::Logger
