//
// Created by TD on 2022/4/28.
//
#pragma once
#include <doodle_core_export.h>
#include <doodle_core/doodle_macro.h>
#include <doodle_core/exception/exception.h>
#include <doodle_core/configure/config.h>
#include <doodle_core/doodle_core_pch.h>
#include <doodle_core/lib_warp/cmrcWarp.h>
#include <date/date.h>
#include <entt/entt.hpp>
#include <doodle_core/lib_warp/json_warp.h>
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
DOODLE_CORE_EXPORT std::string file_hash_sha224(const path &in_file);
DOODLE_CORE_EXPORT std::vector<path> list_files(const path &in_dir);
DOODLE_CORE_EXPORT bool is_sub_path(const path &in_parent, const path &in_child);

}  // namespace FSys

}  // namespace doodle
