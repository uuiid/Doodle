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

#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>

#if defined(LOGGER_LIBRARY)
#define LOGGER_API __declspec(dllexport)
#else
#define LOGGER_API __declspec(dllimport)
#endif

#define DOODLE_LOG_DEBUG(message)                                             \
  BOOST_LOG_TRIVIAL(debug)                                                    \
      << "  " << __FILE__ << "  " << __FUNCTION__ << "  " << __LINE__ << "  " \
      << message;

#define DOODLE_LOG_INFO(message)                                              \
  BOOST_LOG_TRIVIAL(info)                                                     \
      << "  " << __FILE__ << "  " << __FUNCTION__ << "  " << __LINE__ << "  " \
      << message;

#define DOODLE_LOG_WARN(message)                                              \
  BOOST_LOG_TRIVIAL(warning)                                                  \
      << "  " << __FILE__ << "  " << __FUNCTION__ << "  " << __LINE__ << "  " \
      << message;

#define DOODLE_LOG_ERROR(message)                                             \
  BOOST_LOG_TRIVIAL(error)                                                    \
      << "  " << __FILE__ << "  " << __FUNCTION__ << "  " << __LINE__ << "  " \
      << message;

namespace Logger {

// #define DOODLE_LOG_DEBUG DOODLE_LOG_DEBUG_ << __FILE__ << __FUNCTION__ << __LINE__ << "\n"
// #define DOODLE_LOG_INFO DOODLE_LOG_INFO_ << __FILE__ << __FUNCTION__ << __LINE__ << "\n"
// #define DOODLE_LOG_WARN DOODLE_LOG_WARN_ << __FILE__ << __FUNCTION__ << __LINE__ << "\n"
// #define DOODLE_LOG_ERROR DOODLE_LOG_ERROR_ << __FILE__ << __FUNCTION__ << __LINE__ << "\n"

void LOGGER_API doodle_initLog(const std::string &logPath = "doodle_%Y_%m_%d_%H_%M.%5N.html",
                               std::size_t logMaxSize = 1024, bool async = true);

}  // namespace Logger
