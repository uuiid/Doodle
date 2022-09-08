//
// Created by TD on 2022/4/28.
//
#pragma once
#include <doodle_core/configure/DOODLE_CORE_API.h>
#include <doodle_core/configure/static_value.h>
#include <doodle_core/doodle_macro.h>
#include <doodle_core/exception/exception.h>

#include <doodle_core/configure/config.h>
#include <doodle_core/doodle_core_pch.h>
#include <date/date.h>
#include <entt/entt.hpp>
#include <doodle_core/core/file_sys.h>
#include <doodle_core/core/chrono_.h>
#include <doodle_core/lib_warp/json_warp.h>
#include <doodle_core/lib_warp/cmrcWarp.h>
#include <doodle_core/lib_warp/entt_warp.h>
#include <doodle_core/lib_warp/enum_template_tool.h>

#include <doodle_core/core/core_help_impl.h>
#include <doodle_core/lib_warp/std_warp.h>
#include <doodle_core/lib_warp/sqlppWarp.h>

namespace boost::asio {
class io_context;
class thread_pool;
}  // namespace boost::asio

// #include <>
namespace doodle {
class convert;
class doodle_error;
class error_iterator;
class nullptr_error;
class serialization_error;
class file_error;

using namespace std::literals;
using namespace date::literals;

class logger_ctrl;
class doodle_lib;
class thread_pool;
using handle_list     = std::vector<entt::handle>;
using logger_ctr_ptr  = std::shared_ptr<logger_ctrl>;
using string_list     = std::vector<std::string>;

using doodle_lib_ptr  = std::shared_ptr<doodle_lib>;
using thread_pool_ptr = std::shared_ptr<thread_pool>;
using registry_ptr    = std::shared_ptr<entt::registry>;

using uuid            = boost::uuids::uuid;

namespace pool_n {
class bounded_limiter;
class null_limiter;
}  // namespace pool_n

template <typename Delta, typename Timiter = pool_n::null_limiter>
class DOODLE_CORE_API scheduler;

template <class Derived>
using process_t      = entt::process<Derived, std::chrono::system_clock::duration>;
using scheduler_t    = scheduler<std::chrono::system_clock::duration>;

using bounded_pool_t = scheduler<std::chrono::system_clock::duration, pool_n::bounded_limiter>;

DOODLE_CORE_API registry_ptr& g_reg();
DOODLE_CORE_API scheduler_t& g_main_loop();
DOODLE_CORE_API bounded_pool_t& g_bounded_pool();
DOODLE_CORE_API thread_pool& g_thread_pool();
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
}  // namespace doodle
