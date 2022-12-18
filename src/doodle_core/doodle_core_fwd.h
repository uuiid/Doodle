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
#include <doodle_core/doodle_core_pch.h>
#include <doodle_core/doodle_macro.h>
#include <doodle_core/exception/exception.h>
#include <doodle_core/lib_warp/cmrcWarp.h>
#include <doodle_core/lib_warp/entt_warp.h>
#include <doodle_core/lib_warp/enum_template_tool.h>
#include <doodle_core/lib_warp/json_warp.h>
#include <doodle_core/lib_warp/sqlppWarp.h>
#include <doodle_core/lib_warp/std_warp.h>

#include <date/date.h>
#include <entt/entt.hpp>

namespace boost::asio {
class io_context;
class thread_pool;
}  // namespace boost::asio

// #include <>
namespace doodle {
class convert;
class doodle_error;
class nullptr_error;
class serialization_error;
class file_error;
class user_ref;

using namespace std::literals;
using namespace date::literals;

class logger_ctrl;
class doodle_lib;
class program_options;
using logger_ctr_ptr = std::shared_ptr<logger_ctrl>;
using string_list    = std::vector<std::string>;

using doodle_lib_ptr = std::shared_ptr<doodle_lib>;
using registry_ptr   = std::shared_ptr<entt::registry>;

using uuid           = boost::uuids::uuid;

namespace pool_n {}  // namespace pool_n

template <class Derived>
using process_t = entt::process<Derived, std::chrono::system_clock::duration>;

DOODLE_CORE_API boost::asio::io_context& g_io_context();
DOODLE_CORE_API boost::asio::thread_pool& g_thread();

namespace movie {
class image_attr;
class image_watermark;
}  // namespace movie

namespace database_n {
class insert;
class select;
class update_data;
class delete_data;
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
namespace dingding {
class user;
}
};  // namespace doodle
