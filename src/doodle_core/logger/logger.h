/*
 * @Author: your name
 * @Date: 2020-10-10 13:18:58
 * @LastEditTime: 2020-10-10 13:48:07
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \Doodle\logger\logger.h
 */
#pragma once

#include <doodle_core/configure/config.h>
#include <doodle_core/configure/doodle_core_export.h>
#include <doodle_core/core/file_sys.h>
#include <doodle_core/doodle_core_pch.h>

#include <spdlog/async.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/spdlog.h>
namespace doodle {

namespace details { /**
                     * @brief 在程序初始化时log就最先运行, 但是输出在了临时文件位置中,
                     */
class DOODLE_CORE_API logger_ctrl {
  FSys::path p_log_path;
  using file_sink_mt_ptr = std::shared_ptr<spdlog::sinks::sink>;
  using async_logger_ptr = std::shared_ptr<spdlog::async_logger>;

 private:
  async_logger_ptr make_log(const FSys::path& in_path, const std::string& in_name);
  void init_temp_log();

 public:
  logger_ctrl();
  virtual ~logger_ctrl();

  file_sink_mt_ptr rotating_file_sink_{};
#if !defined(NDEBUG)
  std::shared_ptr<spdlog::sinks::sink> debug_sink_{};
#endif
  async_logger_ptr make_log(const std::string& in_name, bool out_console = false);
  async_logger_ptr make_log(const FSys::path& in_path, const std::string& in_name, bool out_console);
  async_logger_ptr make_log_file(const FSys::path& in_path, const std::string& in_name, bool out_console = false);

  file_sink_mt_ptr make_file_sink_mt(const std::string& in_name);
  bool add_log_sink(const std::shared_ptr<spdlog::sinks::sink>& in_ptr, const std::string& in_name);
};
}  // namespace details
inline ::spdlog::source_loc log_loc(
    ::spdlog::source_loc const& in_loc = {__builtin_FILE(), __builtin_LINE(), __builtin_FUNCTION()}
) {
  return in_loc;
}
inline void log_debug(
    const std::shared_ptr<spdlog::logger>& in_logger, const std::string& in_msg,
    ::spdlog::source_loc const& in_loc = {__builtin_FILE(), __builtin_LINE(), __builtin_FUNCTION()}
) {
  in_logger->log(in_loc, spdlog::level::debug, in_msg);
}
inline void log_info(
    const std::shared_ptr<spdlog::logger>& in_logger, const std::string& in_msg,
    ::spdlog::source_loc const& in_loc = {__builtin_FILE(), __builtin_LINE(), __builtin_FUNCTION()}
) {
  in_logger->log(in_loc, spdlog::level::info, in_msg);
}

inline void log_warn(
    const std::shared_ptr<spdlog::logger>& in_logger, const std::string& in_msg,
    ::spdlog::source_loc const& in_loc = {__builtin_FILE(), __builtin_LINE(), __builtin_FUNCTION()}
) {
  in_logger->log(in_loc, spdlog::level::warn, in_msg);
}
inline void log_error(
    const std::shared_ptr<spdlog::logger>& in_logger, const std::string& in_msg,
    ::spdlog::source_loc const& in_loc = {__builtin_FILE(), __builtin_LINE(), __builtin_FUNCTION()}
) {
  in_logger->log(in_loc, spdlog::level::err, in_msg);
}
inline void log_debug(
    const std::string& in_msg,
    ::spdlog::source_loc const& in_loc = {__builtin_FILE(), __builtin_LINE(), __builtin_FUNCTION()}
) {
  spdlog::default_logger_raw()->log(in_loc, spdlog::level::debug, in_msg);
}
inline void log_info(
    const std::string& in_msg,
    ::spdlog::source_loc const& in_loc = {__builtin_FILE(), __builtin_LINE(), __builtin_FUNCTION()}
) {
  spdlog::default_logger_raw()->log(in_loc, spdlog::level::info, in_msg);
}

inline void log_warn(
    const std::string& in_msg,
    ::spdlog::source_loc const& in_loc = {__builtin_FILE(), __builtin_LINE(), __builtin_FUNCTION()}
) {
  spdlog::default_logger_raw()->log(in_loc, spdlog::level::warn, in_msg);
}
inline void log_error(
    const std::string& in_msg,
    ::spdlog::source_loc const& in_loc = {__builtin_FILE(), __builtin_LINE(), __builtin_FUNCTION()}
) {
  spdlog::default_logger_raw()->log(in_loc, spdlog::level::err, in_msg);
}
using spdlog::default_logger_raw;
}  // namespace doodle
#define DOODLE_LOG_DEBUG(...) SPDLOG_DEBUG(__VA_ARGS__)

#define DOODLE_LOG_INFO(...) SPDLOG_INFO(__VA_ARGS__)

#define DOODLE_LOG_WARN(...) SPDLOG_WARN(__VA_ARGS__)

#define DOODLE_LOG_ERROR(...) SPDLOG_ERROR(__VA_ARGS__)
