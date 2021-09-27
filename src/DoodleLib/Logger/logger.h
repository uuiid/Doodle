/*
 * @Author: your name
 * @Date: 2020-10-10 13:18:58
 * @LastEditTime: 2020-10-10 13:48:07
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \Doodle\logger\Logger.h
 */
#pragma once
// #include <QDebug>
#include <DoodleConfig.h>
#include <spdlog/spdlog.h>

#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <doodlelib_export.h>

#define DOODLE_LOG_DEBUG(...) \
  SPDLOG_INFO(__VA_ARGS__);

#define DOODLE_LOG_INFO(...) \
  SPDLOG_INFO(__VA_ARGS__);

#define DOODLE_LOG_WARN(...) \
  SPDLOG_WARN(__VA_ARGS__);

#define DOODLE_LOG_ERROR(...) \
  SPDLOG_ERROR(__VA_ARGS__);

namespace doodle::logger {

void DOODLELIB_API doodle_initLog(const std::string &logPath = "doodle",
                                  std::size_t logMaxSize = 16 * 1024 * 1024, bool async = true);

void DOODLELIB_API clear();
}  // namespace doodle::Logger
