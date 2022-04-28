#pragma once

#include <doodle_lib/configure/config.h>
#include <doodle_lib_export.h>

#include <doodle_lib/doodle_macro.h>
#include <doodle_lib/exception/exception.h>
#include <doodle_core/lib_warp/cmrcWarp.h>
#include <doodle_lib/lib_warp/json_warp.h>
#include <doodle_lib/lib_warp/sqlppWarp.h>
#include <doodle_lib/lib_warp/std_warp.h>
#include <doodle_lib/logger/logger.h>
#include <doodle_lib/configure/static_value.h>
#include <doodle_lib/core/core_help_impl.h>
#include <doodle_lib/core/template_util.h>
#include <doodle_lib/lib_warp/icon_font_macro.h>

//开始我们的名称空间
namespace doodle {
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

DOODLELIB_API std::time_t last_write_time_t(const path &in_path);
DOODLELIB_API chrono::sys_time_pos last_write_time_point(const path &in_path);
DOODLELIB_API void last_write_time_point(const path &in_path, const std::chrono::system_clock::time_point &in_time_point);
DOODLELIB_API path add_time_stamp(const path &in_path);
DOODLELIB_API void open_explorer(const path &in_path);
DOODLELIB_API void backup_file(const path &source);
DOODLELIB_API std::string file_hash_sha224(const path &in_file);
DOODLELIB_API std::vector<path> list_files(const path &in_dir);
DOODLELIB_API bool is_sub_path(const path &in_parent, const path &in_child);

}  // namespace FSys

using namespace entt::literals;
using namespace std::literals;
using namespace chrono::literals;

class core_set;
class project;
class episodes;
class shot;
class assets;
class core_sql;
class assets_file;
class time_point_wrap;
class comment;
class doodle_lib;
class thread_pool;
class season;
class setting_windows;
class core_sig;
class app;
class program_options;
class logger_ctrl;
class comm_video;
class short_cut;
class opencv_read_player;
class opencv_player_widget;
class image_icon;
class database_task_select;
class database_task_update;
class database_task_delete;
class database_task_install;
class database_task_obs;

using handle_list         = std::vector<entt::handle>;
using logger_ctr_ptr      = std::shared_ptr<logger_ctrl>;
using program_options_ptr = std::shared_ptr<program_options>;
using conn_ptr            = std::unique_ptr<sqlpp::sqlite3::connection>;
using string_list         = std::vector<std::string>;

using doodle_lib_ptr      = std::shared_ptr<doodle_lib>;
using thread_pool_ptr     = std::shared_ptr<thread_pool>;
using registry_ptr        = std::shared_ptr<entt::registry>;

using uuid                = boost::uuids::uuid;

namespace pool_n {
class bounded_limiter;
class null_limiter;
}  // namespace pool_n

template <typename Delta, typename Timiter = pool_n::null_limiter>
class DOODLELIB_API scheduler;

template <class Derived>
using process_t      = entt::process<Derived, std::chrono::system_clock::duration>;
using scheduler_t    = scheduler<std::chrono::system_clock::duration>;
using bounded_pool_t = scheduler<std::chrono::system_clock::duration, pool_n::bounded_limiter>;

DOODLELIB_API registry_ptr &g_reg();
DOODLELIB_API scheduler_t &g_main_loop();
DOODLELIB_API bounded_pool_t &g_bounded_pool();
DOODLELIB_API thread_pool &g_thread_pool();

constexpr static const null_fun_t null_fun{};
using string = std::string;
}  // namespace doodle
