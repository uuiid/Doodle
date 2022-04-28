//
// Created by TD on 2022/4/28.
//
#pragma once
#include <doodle_core/configure/doodle_core_export.h>
#include <doodle_core/doodle_macro.h>
#include <doodle_core/exception/exception.h>
#include <doodle_core/configure/config.h>
#include <doodle_core/doodle_core_pch.h>
#include <doodle_core/lib_warp/cmrcWarp.h>
#include <date/date.h>
#include <entt/entt.hpp>
#include <doodle_core/lib_warp/json_warp.h>
#include <doodle_core/lib_warp/entt_warp.h>
#include <doodle_core/core/core_help_impl.h>
#include <doodle_core/configure/static_value.h>
#include <doodle_core/lib_warp/std_warp.h>
//#include <>
namespace doodle {
class convert;
class doodle_error;
class error_iterator;
class nullptr_error;
class serialization_error;
class component_error;
class file_error;
namespace chrono {
namespace literals {
using namespace std::chrono_literals;
using namespace date::literals;

}  // namespace literals
using namespace std::chrono;
using namespace date;

using hours_double   = duration<std::double_t, std::ratio<3600>>;
using days_double    = duration<std::double_t, std::ratio<28800>>;
using sys_time_pos   = time_point<system_clock>;
using local_time_pos = time_point<local_t, seconds>;

/**
 * @brief 判断是否是休息日 周六日
 *
 * @todo 这里我们暂时使用周六和周日作为判断, 但是实际上还有各种假期和其他情况要计入
 */
bool is_rest_day(const sys_days &in_days);
bool is_rest_day(const local_days &in_days);
template <class dur>
std::time_t to_time_t(const time_point<local_t, dur> &in_timePoint) {
  return duration_cast<seconds>(in_timePoint.time_since_epoch()).count();
};
}  // namespace chrono

namespace FSys {
using namespace std::filesystem;
using fstream  = std::fstream;
using istream  = std::istream;
using ifstream = std::ifstream;
using ofstream = std::ofstream;
using ostream  = std::ostream;

DOODLE_CORE_EXPORT std::time_t last_write_time_t(const path &in_path);
DOODLE_CORE_EXPORT chrono::sys_time_pos last_write_time_point(const path &in_path);
DOODLE_CORE_EXPORT void last_write_time_point(const path &in_path, const std::chrono::system_clock::time_point &in_time_point);
DOODLE_CORE_EXPORT path add_time_stamp(const path &in_path);
DOODLE_CORE_EXPORT void open_explorer(const path &in_path);
DOODLE_CORE_EXPORT void backup_file(const path &source);
DOODLE_CORE_EXPORT std::vector<path> list_files(const path &in_dir);
DOODLE_CORE_EXPORT bool is_sub_path(const path &in_parent, const path &in_child);

}  // namespace FSys

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
class DOODLE_CORE_EXPORT scheduler;

template <class Derived>
using process_t      = entt::process<Derived, std::chrono::system_clock::duration>;
using scheduler_t    = scheduler<std::chrono::system_clock::duration>;
using bounded_pool_t = scheduler<std::chrono::system_clock::duration, pool_n::bounded_limiter>;

DOODLE_CORE_EXPORT registry_ptr &g_reg();
DOODLE_CORE_EXPORT scheduler_t &g_main_loop();
DOODLE_CORE_EXPORT bounded_pool_t &g_bounded_pool();
DOODLE_CORE_EXPORT thread_pool &g_thread_pool();

}  // namespace doodle
