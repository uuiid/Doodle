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

#include <spdlog/spdlog.h>

namespace doodle {

/**
 * @brief 在程序初始化时log就最先运行, 但是输出在了临时文件位置中,
 */
class DOODLE_CORE_API logger_ctrl {
  FSys::path p_log_path;
  std::string p_log_name;

  void init_temp_log();

  void init_log();

 public:
  logger_ctrl();
  virtual ~logger_ctrl();

  enum log_type {
    none   = 0,
    guiexe = 1,
    server = 2,
  };
  static logger_ctrl& get_log();

  bool add_log_sink(const std::shared_ptr<spdlog::sinks::sink>& in_ptr);

  void refresh();
};

}  // namespace doodle
#define DOODLE_LOG_DEBUG(...) SPDLOG_DEBUG(__VA_ARGS__)

#define DOODLE_LOG_INFO(...) SPDLOG_INFO(__VA_ARGS__)

#define DOODLE_LOG_WARN(...) SPDLOG_WARN(__VA_ARGS__)

#define DOODLE_LOG_ERROR(...) SPDLOG_ERROR(__VA_ARGS__)
