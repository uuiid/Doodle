//
// Created by TD on 2022/4/28.
//
#pragma once
#include <doodle_core/configure/config.h>
#include <doodle_core/configure/doodle_core_export.h>
#include <doodle_core/configure/static_value.h>
#include <doodle_core/core/chrono_.h>
#include <doodle_core/core/core_help_impl.h>
#include <doodle_core/core/file_sys.h>
#include <doodle_core/core/global_function.h>
#include <doodle_core/doodle_core_pch.h>
#include <doodle_core/doodle_macro.h>
#include <doodle_core/exception/exception.h>
#include <doodle_core/lib_warp/boost_fmt_beast.h>
#include <doodle_core/lib_warp/boost_fmt_string.h>
#include <doodle_core/lib_warp/cmrcWarp.h>
#include <doodle_core/lib_warp/entt_warp.h>
#include <doodle_core/lib_warp/enum_template_tool.h>
#include <doodle_core/lib_warp/json_warp.h>
#include <doodle_core/lib_warp/std_warp.h>

#include <entt/entt.hpp>
#include <spdlog/async.h>
#include <spdlog/common.h>
#include <spdlog/logger.h>

namespace boost::asio {
class io_context;
class thread_pool;
}  // namespace boost::asio

namespace spdlog {
class logger;
SPDLOG_API spdlog::logger* default_logger_raw();
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
using entt::literals::operator""_hs;
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

namespace http::detail {
// class http_client_core;
}
namespace details {
class database_info;
class program_info;
class identifier;
class logger_ctrl;
}  // namespace details
namespace detail {
template <typename>
class one_file_base;

template <typename Id, typename Registry = entt::registry, typename... Scope>
struct entt_handle_ref : entt::basic_handle<Registry, Scope...> {
  using base_type          = entt::basic_handle<Registry, Scope...>;
  constexpr static auto id = Id::value;
  explicit entt_handle_ref(const base_type& in_base) : base_type(in_base) {}
  using base_type::base_type;
  using base_type::operator bool;
  using base_type::operator entt::entity;

  entt_handle_ref& operator=(const entt_handle_ref& in_ref) = default;
  entt_handle_ref& operator=(const base_type& in_ref) {
    base_type::operator=(in_ref);
    return *this;
  }
};

constexpr auto ue_path_id     = "ue_path.id"_hs;
constexpr auto rig_path_id    = "rig_path.id"_hs;
constexpr auto solve_path_id  = "solve_path.id"_hs;
constexpr auto project_ref_id = "project.ref.id"_hs;
constexpr auto project_id     = "project.id"_hs;
constexpr auto sql_id         = "sql.id"_hs;

class connect_video_interface;

}  // namespace detail

using database_info = details::database_info;
using program_info  = details::program_info;
using identifier    = details::identifier;

class convert;
class doodle_error;
class user_ref;
class user;
class project;
class comment;
class assets;

using computer_ref = detail::entt_handle_ref<entt::tag<"computer"_hs>>;
using task_ref     = detail::entt_handle_ref<entt::tag<"task"_hs>>;

class ue_main_map;

using connect_video = std::shared_ptr<detail::connect_video_interface>;

using namespace std::literals;

class doodle_lib;
using string_list    = std::vector<std::string>;

using doodle_lib_ptr = std::shared_ptr<doodle_lib>;
using registry_ptr   = std::shared_ptr<entt::registry>;

using uuid           = boost::uuids::uuid;
using spdlog::default_logger_raw;
namespace pool_n {}  // namespace pool_n

DOODLE_CORE_API boost::asio::io_context& g_io_context();
DOODLE_CORE_API boost::asio::thread_pool& g_thread();
DOODLE_CORE_API entt::registry::context& g_ctx();

namespace movie {
class image_attr;
class image_watermark;
}  // namespace movie

namespace database_n {

class select;

template <typename>
struct sql_com;

template <typename>
class impl_obs;
template <typename>
struct sql_ctx;
}  // namespace database_n

using sql_connection     = sqlpp::sqlite3::common_connection;
using sql_connection_ptr = std::shared_ptr<sql_connection>;

namespace json_rpc {
class server;
class parser_rpc;
class rpc_server;
class rpc_server_ref;
class session_manager;
}  // namespace json_rpc

namespace business {
class work_clock;
class work_clock2;
}  // namespace business
};  // namespace doodle
