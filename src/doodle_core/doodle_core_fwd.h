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
#include <doodle_core/lib_warp/sqlppWarp.h>
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
using maya_file_id         = entt::tag<"maya_file"_hs>;
using ue_file_id           = entt::tag<"ue_file"_hs>;
using maya_rig_file_id     = entt::tag<"maya_rig_file"_hs>;
using ue_file_preset_id    = entt::tag<"ue_file_preset"_hs>;
using file_association_ref = entt::tag<"file_association_ref"_hs>;

// 场景id
using scene_id             = entt::tag<"scene"_hs>;
// 人物id
using character_id         = entt::tag<"character"_hs>;
// 道具id
using prop_id              = entt::tag<"prop"_hs>;
// rig id
using rig_id               = entt::tag<"rig"_hs>;
// 动画id
using animation_id         = entt::tag<"animation"_hs>;
// 解算id
using simulation_id        = entt::tag<"simulation"_hs>;

class connect_video_interface;

}  // namespace detail

using database_info = details::database_info;
using program_info  = details::program_info;
using identifier    = details::identifier;
class maya_anim_file;

class convert;
class doodle_error;
class user_ref;
class user;
class project;
class comment;
class assets;
using maya_file            = detail::one_file_base<detail::maya_file_id>;
using ue_file              = detail::one_file_base<detail::ue_file_id>;
using maya_rig_file        = detail::one_file_base<detail::maya_rig_file_id>;
using ue_file_preset       = detail::one_file_base<detail::ue_file_preset_id>;

using file_association_ref = detail::entt_handle_ref<detail::file_association_ref>;
using computer_ref         = detail::entt_handle_ref<entt::tag<"computer"_hs>>;
using task_ref             = detail::entt_handle_ref<entt::tag<"task"_hs>>;
using main_project         = entt::tag<"main_project"_hs>;
class ue_main_map;

using scene_id      = detail::scene_id;
using character_id  = detail::character_id;
using prop_id       = detail::prop_id;
using rig_id        = detail::rig_id;
using animation_id  = detail::animation_id;
using simulation_id = detail::simulation_id;
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

using conn_ptr = std::unique_ptr<sqlpp::sqlite3::connection>;
using pooled_connection = sqlpp::sqlite3::pooled_connection;

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
}
};  // namespace doodle
