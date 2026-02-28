//
// Created by TD on 2022/4/28.
//
#pragma once
#include <doodle_core/configure/config.h>
#include <doodle_core/configure/doodle_core_export.h>
#include <doodle_core/configure/static_value.h>
#include <doodle_core/doodle_core_pch.h>
#include <doodle_core/doodle_macro.h>
#include <doodle_core/exception/exception.h>

#include <boost/uuid/uuid.hpp>

#include <entt/entt.hpp>
#include <spdlog/common.h>
#include <spdlog/logger.h>
#include <spdlog/spdlog.h>
namespace boost::asio {
class io_context;
class thread_pool;
}  // namespace boost::asio

namespace doodle::chrono {
using namespace std::chrono;
namespace literals {
using namespace std::chrono_literals;
}  // namespace literals
using namespace std::chrono;

using hours_double      = duration<std::double_t, std::ratio<3600>>;
using sys_time_pos      = time_point<system_clock>;
using local_time_pos    = time_point<local_t, seconds>;
using system_zoned_time = zoned_time<system_clock::duration>;
template <class dur>
std::time_t to_time_t(const time_point<local_t, dur>& in_timePoint) {
  return duration_cast<seconds>(in_timePoint.time_since_epoch()).count();
};
};  // namespace doodle::chrono

namespace spdlog {
class logger;
SPDLOG_API logger* default_logger_raw();
namespace level {}  // namespace level
}  // namespace spdlog

namespace doodle::FSys {
using namespace std::filesystem;
using fstream  = std::fstream;
using istream  = std::istream;
using ifstream = std::ifstream;
using ofstream = std::ofstream;
using ostream  = std::ostream;
}  // namespace doodle::FSys

#define DOODLE_TO_EXECUTOR(executor_)                             \
  auto this_executor = co_await boost::asio::this_coro::executor; \
  co_await boost::asio::post(boost::asio::bind_executor(executor_, boost::asio::use_awaitable));

#define DOODLE_TO_MAIN_THREAD() DOODLE_TO_EXECUTOR(g_strand())

#define DOODLE_TO_SELF() \
  co_await boost::asio::post(boost::asio::bind_executor(this_executor, boost::asio::use_awaitable));

#define G_DETACHED_LOG(...)                                                \
  [__VA_ARGS__](std::exception_ptr in_ptr) {                               \
    try {                                                                  \
      if (in_ptr) std::rethrow_exception(in_ptr);                          \
    } catch (const boost::system::system_error& in_err) {                  \
      if (in_err.code() == boost::asio::error::connection_aborted) return; \
      if (in_err.code() == boost::asio::error::operation_aborted) return;  \
                                                                           \
      SPDLOG_WARN(boost::current_exception_diagnostic_information());      \
    } catch (...) {                                                        \
      SPDLOG_WARN(boost::current_exception_diagnostic_information());      \
    };                                                                     \
  }

// #include <>
namespace doodle {
using logger_ptr = std::shared_ptr<spdlog::logger>;
namespace level {
using spdlog::level::critical;
using spdlog::level::debug;
using spdlog::level::err;
using spdlog::level::info;
using spdlog::level::level_enum;
using spdlog::level::off;
using spdlog::level::trace;
using spdlog::level::warn;
}  // namespace level

namespace details {
class logger_ctrl;
}  // namespace details

class doodle_error;
class user;
class doodle_lib;

using doodle_lib_ptr = std::shared_ptr<doodle_lib>;

using uuid           = boost::uuids::uuid;
using spdlog::default_logger_raw;

using namespace std::literals;

};  // namespace doodle
