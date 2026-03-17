#pragma once

#include <doodle_core/doodle_core.h>
#include <doodle_core/doodle_core_fwd.h>

#include <doodle_lib/configure/doodle_lib_export.h>
#include <doodle_lib/core/global_function.h>
#include <doodle_lib/doodle_lib_pch.h>

#include <boost/asio.hpp>

#include <entt/entt.hpp>
#include <spdlog/common.h>
#include <spdlog/logger.h>
#include <spdlog/spdlog.h>

#define DOODLE_TO_EXECUTOR(executor_)                             \
  auto this_executor = co_await boost::asio::this_coro::executor; \
  co_await boost::asio::post(boost::asio::bind_executor(executor_, boost::asio::use_awaitable));

#define DOODLE_TO_MAIN_THREAD() DOODLE_TO_EXECUTOR(g_strand())

#define DOODLE_TO_SELF() \
  co_await boost::asio::post(boost::asio::bind_executor(this_executor, boost::asio::use_awaitable));

namespace spdlog {
class logger;
SPDLOG_API logger* default_logger_raw();
namespace level {}  // namespace level
}  // namespace spdlog

// 开始我们的名称空间
namespace doodle {

namespace socket_io {
class sid_ctx;
}

namespace details {
class logger_ctrl;
}  // namespace details

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
using spdlog::default_logger_raw;

DOODLELIB_API boost::asio::io_context& g_io_context();
DOODLELIB_API entt::registry::context& g_ctx();

class doodle_lib;
class doodle_lib;
class sqlite_database;
class server_task_info;
using doodle_lib_ptr = std::shared_ptr<doodle_lib>;

namespace movie {
class image_attr;
class image_watermark;
}  // namespace movie

namespace business {
class work_clock2;
}  // namespace business
namespace FSys {
DOODLELIB_API std::string file_hash_sha224(const path& in_file);
}

namespace render_farm {
namespace detail {
struct basic_json_body;
class http_route;
class ue4_task;
class render_ue4;

}  // namespace detail
using render_ue4     = detail::render_ue4;
using render_ue4_ptr = std::shared_ptr<render_ue4>;
using http_route_ptr = std::shared_ptr<detail::http_route>;

}  // namespace render_farm

namespace detail {
struct process_child;
}
using process_child_ptr = std::shared_ptr<detail::process_child>;
using namespace entt::literals;
using namespace std::literals;
using namespace chrono::literals;

class core_set;
class project;
class episodes;
class shot;
class assets;
class assets_file;
class time_point_wrap;
class season;

class core_sig;
class opencv_read_player;
class opencv_player_widget;
class image_icon;

class holidaycn_time;
class udp_client;
using udp_client_ptr = std::shared_ptr<udp_client>;

namespace http {
class http_work;
}
}  // namespace doodle
