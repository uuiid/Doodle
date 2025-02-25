//
// Created by TD on 2022/4/28.
//
#pragma once
#include <doodle_core/configure/config.h>
#include <doodle_core/configure/doodle_core_export.h>
#include <doodle_core/configure/static_value.h>
#include <doodle_core/core/chrono_.h>
#include <doodle_core/core/file_sys.h>
#include <doodle_core/core/global_function.h>
#include <doodle_core/doodle_core_pch.h>
#include <doodle_core/doodle_macro.h>
#include <doodle_core/exception/exception.h>
#include <doodle_core/lib_warp/boost_fmt_beast.h>
#include <doodle_core/lib_warp/boost_fmt_string.h>
#include <doodle_core/lib_warp/enum_template_tool.h>
#include <doodle_core/lib_warp/json_warp.h>
#include <doodle_core/lib_warp/std_warp.h>

#include <entt/entt.hpp>
#include <spdlog/common.h>
#include <spdlog/logger.h>
namespace boost::asio {
class io_context;
class thread_pool;
}  // namespace boost::asio

namespace spdlog {
class logger;
SPDLOG_API logger* default_logger_raw();
namespace level {}  // namespace level
}  // namespace spdlog

#define DOODLE_TO_EXECUTOR(executor_)                             \
  auto this_executor = co_await boost::asio::this_coro::executor; \
  co_await boost::asio::post(boost::asio::bind_executor(executor_, boost::asio::use_awaitable));

#define DOODLE_TO_MAIN_THREAD() DOODLE_TO_EXECUTOR(g_strand())

#define DOODLE_TO_SELF() \
  co_await boost::asio::post(boost::asio::bind_executor(this_executor, boost::asio::use_awaitable));

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
namespace detail {
template <typename>
class one_file_base;
}  // namespace detail

class doodle_error;
class user;
class doodle_lib;

using doodle_lib_ptr = std::shared_ptr<doodle_lib>;

using uuid           = boost::uuids::uuid;
using spdlog::default_logger_raw;

using namespace std::literals;

DOODLE_CORE_API boost::asio::io_context& g_io_context();
DOODLE_CORE_API entt::registry::context& g_ctx();

namespace movie {
class image_attr;
class image_watermark;
}  // namespace movie

namespace business {
class work_clock2;
}  // namespace business
};  // namespace doodle
