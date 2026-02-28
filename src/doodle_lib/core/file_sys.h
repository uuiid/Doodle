//
// Created by TD on 2022/5/11.
//

#pragma once
#include <doodle_core/configure/doodle_core_export.h>
#include <doodle_core/core/chrono_.h>

#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/uuid/uuid.hpp>

#include <filesystem>
#include <fmt/format.h>
#include <nlohmann/json_fwd.hpp>
#include <optional>
#include <spdlog/logger.h>
namespace doodle::chrono {
using namespace std::chrono;
}

namespace doodle {
using logger_ptr     = std::shared_ptr<spdlog::logger>;
using logger_ptr_raw = spdlog::logger*;
}  // namespace doodle

namespace doodle::FSys {
using namespace std::filesystem;
using fstream  = std::fstream;
using istream  = std::istream;
using ifstream = std::ifstream;
using ofstream = std::ofstream;
using ostream  = std::ostream;

DOODLE_CORE_API std::time_t last_write_time_t(const path& in_path);
DOODLE_CORE_API chrono::sys_time_pos last_write_time_point(const path& in_path);
DOODLE_CORE_API void last_write_time_point(
    const path& in_path, const std::chrono::system_clock::time_point& in_time_point
);
DOODLE_CORE_API path add_time_stamp(const path& in_path);
DOODLE_CORE_API void open_explorer(const path& in_path);
DOODLE_CORE_API void backup_file(const path& source);
DOODLE_CORE_API std::vector<path> list_files(const path& in_dir, const FSys::path& in_extension = {});
DOODLE_CORE_API bool is_sub_path(const path& in_parent, const path& in_child);

FSys::path DOODLE_CORE_API write_tmp_file(
    const std::string& in_falg, const std::string& in_string, const std::string& in_extension,
    const std::optional<std::string>& in_file_name = {}, std::int32_t in_model = std::ios::out
);
FSys::path DOODLE_CORE_API from_quotation_marks(const std::string& in_string);

FSys::path DOODLE_CORE_API get_cache_path();
FSys::path DOODLE_CORE_API get_cache_path(const FSys::path& in_path);

bool DOODLE_CORE_API folder_is_save(const FSys::path& in_file_path);
bool DOODLE_CORE_API is_windows_remote_path(const FSys::path& in_file_path);
// 软件标识符文件
void DOODLE_CORE_API software_flag_file(const FSys::path& in_file_path, const boost::uuids::uuid& in_uuid);
boost::uuids::uuid DOODLE_CORE_API software_flag_file(const FSys::path& in_file_path);

bool DOODLE_CORE_API is_hidden(const FSys::path& in_file_path);
/**
 * 将uuid文件名称 c9f2b8c0-4a1d-4e3b-9f5c-7d6e0a2f3b8d.png 添加前缀 c9f/2b8/c9f2b8c0-4a1d-4e3b-9f5c-7d6e0a2f3b8d.png
 * @param in_file_path 文件名称
 * @return 分裂后重新组合的文件名称
 */
FSys::path split_uuid_path(const FSys::path& in_file_path);
bool copy_diff(const FSys::path& from, const FSys::path& to, logger_ptr in_logger = nullptr);
// 比较两个文件是否不同(简单, 使用文件大小和最后修改时间)
bool DOODLE_CORE_API is_diff(const FSys::path& in_path1, const FSys::path& in_path2);
bool DOODLE_CORE_API is_old_file(const FSys::path& in_path1, const FSys::path& in_path2);

}  // namespace doodle::FSys
