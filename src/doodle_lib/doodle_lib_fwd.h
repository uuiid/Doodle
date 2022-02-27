#pragma once

#include <DoodleConfig.h>
#include <doodle_lib/doodle_lib_pch.h>
#include <doodle_lib/doodle_macro.h>
#include <doodle_lib/exception/exception.h>
#include <doodle_lib/lib_warp/cmrcWarp.h>
#include <doodle_lib/lib_warp/json_warp.h>
#include <doodle_lib/lib_warp/sqlppWarp.h>
#include <doodle_lib/lib_warp/std_warp.h>
#include <doodle_lib/logger/logger.h>
#include <doodle_lib_export.h>
#include <doodle_lib/core/static_value.h>
#include <doodle_lib/core/core_help_impl.h>
//开始我们的名称空间
namespace doodle {
namespace details {
/**
 * @brief 不可复制类
 *
 */
class no_copy {
 public:
  no_copy()                = default;
  no_copy(const no_copy &) = delete;
  no_copy &operator=(const no_copy &) = delete;

  no_copy(no_copy &&)                 = default;
  no_copy &operator=(no_copy &&) = default;
};
/**
 * @brief 判断是否是智能指针
 *
 * @tparam T 需要判断的类型
 */
template <typename T, typename = void>
struct is_smart_pointer : public std::false_type {};
/**
 * @brief 判断是否是智能指针
 *
 * @tparam T 需要判断的类型
 * @tparam Enable 辅助类
 */

template <typename T>
struct is_smart_pointer<T, std::void_t<decltype(T::element_type)>> : public std::true_type {};

/// to boost::less_pointees_t;

}  // namespace details

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
template <class dur>
std::time_t to_time_t(const time_point<local_t, dur> &in_timePoint) {
  return duration_cast<seconds>(in_timePoint.time_since_epoch()).count();
};
// template <class Clock>
// bool is_morning_works(const std::chrono::time_point<Clock, typename Clock::duration>& in_time) {
// }
}  // namespace chrono

template <typename IntType = std::int32_t>
std::vector<IntType> range(IntType start, IntType stop, IntType step = 1) {
  std::vector<IntType> x{};
  boost::push_back(x, boost::irange(start, stop, step));
  return x;
};

namespace FSys {
using namespace std::filesystem;
using fstream  = std::fstream;
using istream  = std::istream;
using ifstream = std::ifstream;
using ofstream = std::ofstream;
using ostream  = std::ostream;
DOODLELIB_API inline path make_path(const std::string &in_string) {
  std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>, wchar_t> convert;
  return path{convert.from_bytes(in_string)};
}
DOODLELIB_API std::time_t last_write_time_t(const path &in_path);
DOODLELIB_API chrono::sys_time_pos last_write_time_point(const path &in_path);
DOODLELIB_API void last_write_time_point(const path &in_path, const std::chrono::system_clock::time_point &in_time_point);
DOODLELIB_API path add_time_stamp(const path &in_path);
DOODLELIB_API void open_explorer(const path &in_path);
DOODLELIB_API void backup_file(const path &source);
DOODLELIB_API std::string file_hash_sha224(const path &in_file);
DOODLELIB_API std::vector<path> list_files(const path &in_dir);

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
class rpc_server_handle;
class doodle_lib;
class thread_pool;
class season;
class setting_windows;
class core_sig;
class command_base;
class rpc_trans_path;
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

using uuid = boost::uuids::uuid;

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

template <class Component,
          std::enable_if_t<!std::is_same_v<entt::entity, Component>, bool> = true>
entt::handle make_handle(const Component &instance) {
  return entt::handle{*(g_reg()), entt::to_entity(*(g_reg()), instance)};
};
template <class Component,
          std::enable_if_t<std::is_same_v<entt::entity, Component>, bool> = true>
entt::handle make_handle(const Component &instance) {
  return entt::handle{*(g_reg()), instance};
};

inline entt::handle make_handle() {
  return entt::handle{*(g_reg()), g_reg()->create()};
};

template <class Component>
entt::entity to_entity(const Component &instance) {
  return entt::to_entity(*(g_reg()), instance);
};

template <class Component_to, class Component_From>
entt::entity to_comm(const Component_From &instance) {
  auto k_h = make_handle(instance);
  chick_true<component_error>(k_h.any_of<Component_to>(), DOODLE_LOC, "缺失组件");
  return entt::to_entity(*(g_reg()), instance);
};

template <class... Component>
void chick_component(const entt::handle &t) {
  chick_true<doodle_error>(t, DOODLE_LOC, "无效句柄");
  chick_true<component_error>(t.any_of<Component...>(), DOODLE_LOC, "缺失组件");
}

template <class Component>
void chick_ctx() {
  chick_true<component_error>(
      g_reg()->template try_ctx<Component>(),
      DOODLE_LOC, "缺失上下文");
}

class DOODLELIB_API null_fun_t {
 public:
  null_fun_t() = default;
  template <class in_class>
  inline void operator()(in_class &in){};
};
static null_fun_t null_fun{};

using setting_windows_ptr     = std::shared_ptr<setting_windows>;

using bool_ptr                = std::shared_ptr<bool>;

using string                  = std::string;
using string_ptr              = std::shared_ptr<string>;

using rpc_trans_path_ptr      = std::unique_ptr<rpc_trans_path>;
using rpc_trans_path_ptr_list = std::vector<rpc_trans_path_ptr>;

namespace gui {
template <class T>
struct adl_traits {};
}  // namespace gui

namespace details {
template <class in_type>
std::pair<string, string> make_show_shr(const string &in_key, const in_type *in_ptr) {
  return std::make_pair(in_key, fmt::format("{}##{}", in_key, typeid(*in_ptr).name()));
};
}  // namespace details
template <class... Args, class in_type>
std::map<string, string> make_imgui_name(const in_type *in_ptr, Args &&...in_args) {
  return std::map<string, string>{details::make_show_shr(in_args, in_ptr)...};
};
}  // namespace doodle
