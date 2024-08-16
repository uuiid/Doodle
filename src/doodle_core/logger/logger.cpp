#include "logger.h"

#include <doodle_core/core/core_set.h>

#include <boost/algorithm/string/erase.hpp>
#include <boost/locale.hpp>
#include <boost/process/environment.hpp>

#include <Windows.h>
#include <chrono>
#include <fmt/core.h>
#include <spdlog/async.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/daily_file_sink.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/stdout_sinks.h>

namespace doodle::details {
template <class Mutex>
class msvc_doodle_sink : public spdlog::sinks::base_sink<Mutex> {
 public:
  msvc_doodle_sink() = default;

 protected:
  void sink_it_(const spdlog::details::log_msg& msg) override {
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

#if !defined(NDEBUG)
#define DOODLE_ADD_DEBUG_SINK(logger) logger->sinks().push_back(debug_sink_);
#else
#define DOODLE_ADD_DEBUG_SINK(logger)
#endif
//
// 我们自己的旋转日志, spd的那个有时候会无法重命名
//
template <typename Mutex>
class rotating_file_sink final : public spdlog::sinks::base_sink<Mutex> {
 public:
  explicit rotating_file_sink(FSys::path in_path, std::size_t max_size, std::size_t max_files = 10);

 protected:
  void sink_it_(const spdlog::details::log_msg& msg) override;
  void flush_() override;

 private:
  // Rotate files:
  // log.txt -> log.1.txt
  // log.1.txt -> log.2.txt
  // log.2.txt -> log.3.txt
  void rotate_();

  std::string file_stem_;
  FSys::path base_filename_;
  std::size_t max_size_;
  std::size_t max_files_;
  std::size_t current_size_;
  std::ofstream file_helper_;
  std::size_t index_;
};

template <typename Mutex>
rotating_file_sink<Mutex>::rotating_file_sink(FSys::path in_path, std::size_t max_size, std::size_t max_files /*= 10*/)
    : file_stem_(in_path.stem().generic_string()),
      base_filename_(std::move(in_path)),
      max_size_(std::clamp(max_size, 2ull, 1024ull * 1024ull * 1024ull * 1024ull)),
      max_files_(std::clamp(max_files, 2ull, 200000ull)),
      current_size_(0),
      index_(1) {
  base_filename_.replace_filename(fmt::format("{}.{}.txt", file_stem_, index_));
  FSys::create_directories(base_filename_.parent_path());
  if (FSys::exists(base_filename_)) current_size_ = FSys::file_size(base_filename_);
  file_helper_.open(base_filename_, std::ios_base::app | std::ios_base::out | std::ios_base::binary);
}

template <typename Mutex>
void rotating_file_sink<Mutex>::sink_it_(const spdlog::details::log_msg& msg) {
  spdlog::memory_buf_t formatted;
  spdlog::sinks::base_sink<Mutex>::formatter_->format(msg, formatted);
  auto new_size = current_size_ + formatted.size();

  // rotate if the new estimated file size exceeds max size.
  // rotate only if the real size > 0 to better deal with full disk (see issue #2261).
  // we only check the real size when new_size > max_size_ because it is relatively expensive.
  if (new_size > max_size_) {
    file_helper_.flush();
    if (FSys::exists(base_filename_)) {
      rotate_();
      new_size = formatted.size();
    }
  }
  file_helper_.write(formatted.data(), formatted.size());
  current_size_ = new_size;
}

template <typename Mutex>
void rotating_file_sink<Mutex>::flush_() {
  file_helper_.flush();
}

template <typename Mutex>
void rotating_file_sink<Mutex>::rotate_() {
  file_helper_ = {};
  //  auto l_target = base_filename_;
  base_filename_.replace_filename(fmt::format("{}.{}.txt", file_stem_, ++index_));
  file_helper_.open(base_filename_, std::ios_base::app | std::ios_base::out | std::ios_base::binary);
  current_size_ = 0;

  // delete existing file
  std::error_code l_ec{};
  if (index_ > max_files_) {
    for (std::size_t i = index_ - max_files_; i > 0; --i) {
      auto l_target = base_filename_;
      l_target.replace_filename(fmt::format("{}.{}.txt", file_stem_, i));
      FSys::remove(l_target, l_ec);
    }
  }
}

using rotating_file_sink_mt = rotating_file_sink<std::mutex>;

logger_ctrl::logger_ctrl()
    : p_log_path(
          FSys::temp_directory_path() / "doodle" / "log" / fmt::format("process_id_{}", boost::this_process::get_id())
      ) {
  spdlog::init_thread_pool(8192, 1);
  init_temp_log();
}

logger_ctrl::async_logger_ptr logger_ctrl::make_log(const FSys::path& in_path, const std::string& in_name) {
  if (!FSys::exists(in_path)) FSys::create_directories(in_path);
  auto l_path = in_path / fmt::format("{}.txt", in_name);
  std::shared_ptr<spdlog::async_logger> l_logger;
  try {
    rotating_file_sink_ = std::make_shared<rotating_file_sink_mt>(l_path, 1024ull * 1024ull * 512ull);
    l_logger            = std::make_shared<spdlog::async_logger>(
        in_name, rotating_file_sink_, spdlog::thread_pool(), spdlog::async_overflow_policy::block
    );

#if !defined(NDEBUG)
    debug_sink_ = std::make_shared<msvc_doodle_sink_mt>();
#endif
    DOODLE_ADD_DEBUG_SINK(l_logger);
    spdlog::register_logger(l_logger);
  } catch (const spdlog::spdlog_ex& spdlog_ex) {
    std::cout << "日志初始化失败" << boost::diagnostic_information(spdlog_ex) << std::endl;
  }
  l_logger->log(log_loc(), spdlog::level::debug, "初始化日志 {}", in_name);

  return l_logger;
}

void logger_ctrl::init_temp_log() {
  auto l_logger = make_log(p_log_path, "doodle_lib");
  spdlog::set_default_logger(l_logger);

  spdlog::flush_every(3s);
  spdlog::flush_on(spdlog::level::warn);
  spdlog::set_level(spdlog::level::trace);
  spdlog::should_log(spdlog::level::trace);
  spdlog::set_pattern("[%l] [%Y-%m-%d %H:%M:%S.%e] [thread %t] [%n] [%s:%#] %v");
}

logger_ctrl::async_logger_ptr logger_ctrl::make_log(const std::string& in_name, bool out_console) {
  auto l_name = in_name;
  std::erase_if(l_name, [](const char& in) -> bool { return !std::isalnum(in) && in != '_' && in != '.'; });

  auto l_path = p_log_path / fmt::format("{}_{}.txt", l_name, core_set::get_set().get_uuid());

  std::vector<spdlog::sink_ptr> l_sinks{};
  l_sinks.emplace_back(std::make_shared<spdlog::sinks::stderr_color_sink_mt>())
      ->set_level(out_console ? spdlog::level::debug : spdlog::level::err);
  l_sinks.emplace_back(std::make_shared<spdlog::sinks::basic_file_sink_mt>(l_path.generic_string(), true));
  l_sinks.emplace_back(rotating_file_sink_);

  auto l_logger = std::make_shared<spdlog::async_logger>(
      in_name, std::begin(l_sinks), std::end(l_sinks), spdlog::thread_pool(), spdlog::async_overflow_policy::block
  );
  l_logger->set_level(spdlog::level::trace);
  l_logger->should_log(spdlog::level::trace);
  DOODLE_ADD_DEBUG_SINK(l_logger);
  return l_logger;
}

logger_ctrl::async_logger_ptr logger_ctrl::make_log(
    const FSys::path& in_path, const std::string& in_name, bool out_console
) {
  auto l_path = p_log_path / in_path / fmt::format("{}.txt", in_name);

  std::vector<spdlog::sink_ptr> l_sinks{};
  l_sinks.emplace_back(std::make_shared<spdlog::sinks::stderr_color_sink_mt>())
      ->set_level(out_console ? spdlog::level::debug : spdlog::level::err);
  // l_sinks.emplace_back(std::make_shared<spdlog::sinks::basic_file_sink_mt>(l_path.generic_string(), true));
  l_sinks.emplace_back(rotating_file_sink_);

  auto l_logger = std::make_shared<spdlog::async_logger>(
      in_name, std::begin(l_sinks), std::end(l_sinks), spdlog::thread_pool(), spdlog::async_overflow_policy::block
  );
  l_logger->set_level(spdlog::level::trace);
  l_logger->should_log(spdlog::level::trace);
  DOODLE_ADD_DEBUG_SINK(l_logger);

  return l_logger;
}

logger_ctrl::async_logger_ptr logger_ctrl::make_log_file(
    const FSys::path& in_path, const std::string& in_name, bool out_console
) {
  auto l_path = p_log_path / fmt::format("{}_{}.txt", in_name, core_set::get_set().get_uuid());

  std::vector<spdlog::sink_ptr> l_sinks{
      // std::make_shared<spdlog::sinks::basic_file_sink_mt>(l_path.generic_string(), true)

  };
  // if (in_path != l_path)
  // l_sinks.emplace_back(std::make_shared<spdlog::sinks::basic_file_sink_mt>(in_path.generic_string(), true));
  l_sinks.emplace_back(rotating_file_sink_);

  l_sinks.emplace_back(std::make_shared<spdlog::sinks::stderr_color_sink_mt>())
      ->set_level(out_console ? spdlog::level::debug : spdlog::level::err);

  auto l_logger = std::make_shared<spdlog::async_logger>(
      in_name, std::begin(l_sinks), std::end(l_sinks), spdlog::thread_pool(), spdlog::async_overflow_policy::block
  );
  DOODLE_ADD_DEBUG_SINK(l_logger);
  return l_logger;
}

logger_ctrl::~logger_ctrl() {
  spdlog::apply_all([](const std::shared_ptr<spdlog::logger>& in_ptr) { in_ptr->flush(); });
  spdlog::shutdown();
}

bool logger_ctrl::add_log_sink(const std::shared_ptr<spdlog::sinks::sink>& in_ptr, const std::string& in_name) {
  auto l_default_logger = spdlog::default_logger_raw();
  auto l_logger         = make_log(p_log_path, in_name);
  l_logger->sinks().emplace_back(in_ptr);
  /// 刷新默认log
  try {
    l_default_logger->flush();
  } catch (const spdlog::spdlog_ex& spdlog_ex) {
    l_logger->log(log_loc(), spdlog::level::err, "刷新旧的日志失败 {}", boost::diagnostic_information(spdlog_ex));
  }

  auto l_name = spdlog::default_logger()->name();
  spdlog::set_default_logger(l_logger);

  /// 去除旧的log
  try {
    spdlog::drop(l_name);
  } catch (const spdlog::spdlog_ex& spdlog_ex) {
    l_logger->log(log_loc(), spdlog::level::err, "删除旧的日志失败 {}", boost::diagnostic_information(spdlog_ex));
  }

  SPDLOG_DEBUG(fmt::format("初始化日志 {}", in_name));
  return true;
}
} // namespace doodle::details