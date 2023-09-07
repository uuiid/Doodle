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
namespace doodle::details {

/**
 * @brief 在程序初始化时log就最先运行, 但是输出在了临时文件位置中,
 */
class DOODLE_CORE_API logger_ctrl {
  FSys::path p_log_path;
  using rotating_file_sink_mt_ptr = std::shared_ptr<spdlog::sinks::rotating_file_sink_mt>;
  using async_logger_ptr          = std::shared_ptr<spdlog::async_logger>;

 private:
  async_logger_ptr make_log(const FSys::path& in_path, const std::string& in_name);
  void init_temp_log();

 public:
  logger_ctrl();
  virtual ~logger_ctrl();

  enum log_type {
    none   = 0,
    guiexe = 1,
    server = 2,
  };

  rotating_file_sink_mt_ptr rotating_file_sink_{};
  async_logger_ptr make_log(const std::string& in_name, bool out_console = false);
  bool add_log_sink(const std::shared_ptr<spdlog::sinks::sink>& in_ptr, const std::string& in_name);
};

}  // namespace doodle::details
#define DOODLE_LOG_DEBUG(...) SPDLOG_DEBUG(__VA_ARGS__)

#define DOODLE_LOG_INFO(...) SPDLOG_INFO(__VA_ARGS__)

#define DOODLE_LOG_WARN(...) SPDLOG_WARN(__VA_ARGS__)

#define DOODLE_LOG_ERROR(...) SPDLOG_ERROR(__VA_ARGS__)
