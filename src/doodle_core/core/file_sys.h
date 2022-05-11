//
// Created by TD on 2022/5/11.
//

#pragma once
#include <filesystem>

#include <fmt/format.h>

#include <nlohmann/json_fwd.hpp>
#include <boost/filesystem.hpp>
#include <doodle_core/core/chrono_.h>
#include <doodle_core/configure/doodle_core_export.h>

namespace doodle::chrono {
using namespace std::chrono;
}

namespace doodle::FSys {
#ifdef USE_STD_FSYS
using namespace std::filesystem;
using fstream  = std::fstream;
using istream  = std::istream;
using ifstream = std::ifstream;
using ofstream = std::ofstream;
using ostream  = std::ostream;
#else

using namespace boost::filesystem;

#endif

DOODLE_CORE_EXPORT std::time_t last_write_time_t(const path& in_path);
DOODLE_CORE_EXPORT chrono::sys_time_pos last_write_time_point(const path& in_path);
DOODLE_CORE_EXPORT void last_write_time_point(const path& in_path, const std::chrono::system_clock::time_point& in_time_point);
DOODLE_CORE_EXPORT path add_time_stamp(const path& in_path);
DOODLE_CORE_EXPORT void open_explorer(const path& in_path);
DOODLE_CORE_EXPORT void backup_file(const path& source);
DOODLE_CORE_EXPORT std::vector<path> list_files(const path& in_dir);
DOODLE_CORE_EXPORT bool is_sub_path(const path& in_parent, const path& in_child);

}  // namespace doodle::FSys
namespace fmt {

}  // namespace fmt
