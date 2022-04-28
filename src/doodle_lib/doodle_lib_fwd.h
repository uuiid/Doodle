#pragma once

#include <doodle_lib/configure/config.h>
#include <doodle_lib_export.h>

#include <doodle_macro.h>

#include <doodle_core/lib_warp/cmrcWarp.h>
#include <lib_warp/json_warp.h>
#include <doodle_lib/lib_warp/sqlppWarp.h>
#include <doodle_lib/lib_warp/std_warp.h>

#include <doodle_lib/configure/static_value.h>
#include <core/core_help_impl.h>
#include <core/template_util.h>
#include <doodle_lib/lib_warp/icon_font_macro.h>

#include <doodle_core/doodle_core_fwd.h>

// 开始我们的名称空间
namespace doodle {

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
