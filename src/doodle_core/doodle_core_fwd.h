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

#include <date/date.h>
#include <entt/entt.hpp>
#include <spdlog/async.h>
#include <spdlog/logger.h>

namespace boost::asio {
class io_context;
class thread_pool;
}  // namespace boost::asio

// #include <>
namespace doodle {
using entt::literals::operator""_hs;
using logger_ptr = std::shared_ptr<spdlog::logger>;
namespace details {
class database_info;
class program_info;
class identifier;
class logger_ctrl;
}  // namespace details
namespace detail {
template <typename>
class one_file_base;
template <typename>
struct entt_id {
  entt::entity id;
  inline operator entt::entity() const { return id; }
  inline operator bool() const { return id != entt::null; }

  // to json
  inline friend void to_json(nlohmann::json& j, const entt_id& p) { j = p.id; }
  inline friend void from_json(const nlohmann::json& j, entt_id& p) { p.id = j.get<entt::entity>(); }
};
template <typename>
struct entt_handle {
  entt::handle handle_;
  inline operator entt::handle() const { return handle_; }
  inline operator bool() const { return static_cast<bool>(handle_); }
};
using maya_file_id      = entt::tag<"maya_file"_hs>;
using ue_file_id        = entt::tag<"ue_file"_hs>;
using maya_rig_file_id  = entt::tag<"maya_rig_file"_hs>;
using ue_file_preset_id = entt::tag<"ue_file_preset"_hs>;
}  // namespace detail

using database_info = details::database_info;
using program_info  = details::program_info;
using identifier    = details::identifier;

class convert;
class doodle_error;
class user_ref;
class user;
class project;
class work_task_info;
class comment;
class assets;
using maya_file      = detail::one_file_base<detail::maya_file_id>;
using ue_file        = detail::one_file_base<detail::ue_file_id>;
using maya_rig_file  = detail::one_file_base<detail::maya_rig_file_id>;
using ue_file_preset = detail::one_file_base<detail::ue_file_preset_id>;

using namespace std::literals;

class doodle_lib;
using string_list    = std::vector<std::string>;

using doodle_lib_ptr = std::shared_ptr<doodle_lib>;
using registry_ptr   = std::shared_ptr<entt::registry>;

using uuid           = boost::uuids::uuid;

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

namespace json_rpc {
class server;
class parser_rpc;
class rpc_server;
class rpc_server_ref;
class session_manager;
}  // namespace json_rpc

namespace business {
class work_clock;
}
};  // namespace doodle
