//
// Created by TD on 2022/5/11.
//
#include "file_sys.h"

#include <doodle_core/core/core_set.h>
#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/logger/logger.h>

#include <boost/algorithm/string/predicate.hpp>
#include <boost/dll.hpp>

#include <Windows.h>
#include <filesystem>
#include <nlohmann/json.hpp>
#include <shellapi.h>
#include <tchar.h>
#include <wil/result.h>

namespace doodle::FSys {

#ifdef USE_STD_FSYS
///  这个在windows上使用
///  以100纳秒为时间单位的时间段
using filetime_duration = std::chrono::duration<std::int64_t, std::ratio<1, 10'000'000>>;
/// 从公元 1601 年到公元 1970 年有 369 年的差异，转换为 11644473600秒
constexpr std::chrono::duration<std::int64_t> nt_to_unix_epoch{INT64_C(-11644473600)};
static constexpr chrono::seconds S_epoch_diff{6437664000};

std::time_t last_write_time_t(const path& in_path) {
  auto k_time = last_write_time(in_path);
#if defined(_WIN32) && defined(_MSC_VER)

  /// 这个在Windows上获得的是FILETIME结构转换的int64值的时间点,
  /// https://docs.microsoft.com/en-us/windows/win32/api/minwinbase/ns-minwinbase-filetime
  /// 我们先把它转换为以100纳秒为单位的时间段,
  const filetime_duration asDuration{static_cast<int64_t>(k_time.time_since_epoch().count())};
  /// 然后我们进行加法运算, 加上 11644473600秒。
  /// 从公元 1601 年到公元 1970 年有 369 年的差异，转换为 11644473600秒
  const auto withUnixEpoch = asDuration + nt_to_unix_epoch;
  /// 然后我们将100纳秒时间段转换为 单位秒的时间段
  /// 最后我们进行强制转换， 将int64值， 转换为time_t值
  return static_cast<time_t>(std::chrono::floor<std::chrono::seconds>(withUnixEpoch).count());
#elif defined(__linux__) && defined(__GNUC__)
  chrono::sys_time_pos k_sys_time_pos{k_time.time_since_epoch()};
  k_sys_time_pos -= S_epoch_diff;
  return chrono::system_clock::to_time_t(k_sys_time_pos);
#endif
}

chrono::sys_time_pos last_write_time_point(const path& in_path) {
  auto k_time = last_write_time(in_path);
#if defined(_WIN32) && defined(_MSC_VER)

  /// 这个在Windows上获得的是FILETIME结构转换的int64值的时间点,
  /// https://docs.microsoft.com/en-us/windows/win32/api/minwinbase/ns-minwinbase-filetime
  /// 我们先把它转换为以100纳秒为单位的时间段,
  const filetime_duration asDuration{static_cast<int64_t>(k_time.time_since_epoch().count())};
  /// 然后我们进行加法运算, 加上 11644473600秒。
  /// 从公元 1601 年到公元 1970 年有 369 年的差异，转换为 11644473600秒
  const auto withUnixEpoch = asDuration + nt_to_unix_epoch;
  /// 最后我们强制转换为时间系统时间点
  return chrono::system_clock::time_point{
      chrono::duration_cast<chrono::system_clock::duration>(chrono::floor<std::chrono::seconds>(withUnixEpoch))
  };
#elif defined(__linux__) && defined(__GNUC__)
  chrono::sys_time_pos k_sys_time_pos{k_time.time_since_epoch()};
  k_sys_time_pos -= S_epoch_diff;
  return k_sys_time_pos;
#endif
}
void last_write_time_point(const path& in_path, const std::chrono::system_clock::time_point& in_time_point) {
  const auto asDuration  = chrono::duration_cast<filetime_duration>(in_time_point.time_since_epoch());
  const auto withNtEpoch = asDuration - nt_to_unix_epoch;
  std::filesystem::file_time_type k_time{withNtEpoch};
  std::filesystem::last_write_time(in_path, k_time);
}
#else
std::time_t last_write_time_t(const path& in_path) { return last_write_time(in_path); }

chrono::sys_time_pos last_write_time_point(const path& in_path) {
  return chrono::sys_time_pos::clock::from_time_t(last_write_time(in_path));
}

void last_write_time_point(const path& in_path, const std::chrono::system_clock::time_point& in_time_point) {
  last_write_time(in_path, std::chrono::system_clock::to_time_t(in_time_point));
}

#endif

void open_explorer(const path& in_path) {
  DOODLE_LOG_INFO("打开路径: {}", in_path.generic_string());
  ShellExecute(
      nullptr, _T("open"), _T("EXPLORER.EXE"), in_path.lexically_normal().native().c_str(), nullptr, SW_SHOWDEFAULT
  );
  // std::system(fmt::format(R"(explorer.exe {})", in_path.generic_string()).c_str());
}

void backup_file(const path& source) {
  auto backup_path = source.parent_path() / "backup" / add_time_stamp(source).filename();
  if (!exists(backup_path.parent_path())) create_directories(backup_path.parent_path());
  copy_file(source, backup_path, copy_options::overwrite_existing);
}
path add_time_stamp(const path& in_path) {
  auto k_fn = in_path.stem();
  k_fn += fmt::format("_{:%Y_%m_%d_%H_%M_%S}", std::chrono::system_clock::now());
  k_fn += in_path.extension();
  auto k_path = in_path.parent_path() / k_fn;
  return k_path;
}

std::vector<path> list_files(const path& in_dir, const FSys::path& in_extension) {
  std::vector<path> l_out_files;
  for (const auto& l_it : directory_iterator{in_dir}) {
    if (l_it.is_regular_file() && (in_extension.empty() || l_it.path().extension() == in_extension)) {
      l_out_files.emplace_back(l_it.path());
    }
  }
  return l_out_files;
}
bool is_sub_path(const path& in_parent, const path& in_child) {
  return boost::istarts_with(in_parent.generic_string(), in_child.generic_string());
}

FSys::path write_tmp_file(
    const std::string& in_falg, const std::string& in_string, const std::string& in_extension,
    const std::optional<std::string>& in_file_name, std::int32_t in_model
) {
  auto tmp_path = core_set::get_set().get_cache_root(
      fmt::format(
          "{}/v{}{}{}", in_falg, version::build_info::get().version_major, version::build_info::get().version_minor,
          version::build_info::get().version_patch
      )
  );
  auto k_tmp_path = tmp_path / FSys::path{
                                   in_file_name ? (*in_file_name + in_extension)
                                                : boost::uuids::to_string(core_set::get_set().get_uuid()) + in_extension
                               };
  if (in_file_name && FSys::exists(k_tmp_path)) {
    return k_tmp_path;
  } else {
    // 写入文件后直接关闭
    FSys::fstream file{k_tmp_path, in_model};
    file << in_string;
  }

  return k_tmp_path;
}

FSys::path from_quotation_marks(const std::string& in_string) {
  if (in_string.empty()) return FSys::path{};
  if (*in_string.begin() == '"' && *(--in_string.end()) == '"') {
    return FSys::path{in_string.substr(1, in_string.size() - 1)};
  }
  return FSys::path{in_string};
}

FSys::path get_cache_path() { return FSys::temp_directory_path() / "Doodle" / "cache"; }
FSys::path get_cache_path(const FSys::path& in_path) {
  auto l_tmp = get_cache_path() / in_path;
  if (!FSys::exists(l_tmp)) FSys::create_directories(l_tmp);
  return l_tmp;
}

bool folder_is_save(const FSys::path& in_file_path) {
// #define DOODLE_FSYS_WIN32
#ifdef DOODLE_FSYS_WIN32
  DWORD l_len{};
  if ((::GetFileSecurityW(
           in_file_path.c_str(), OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION,
           nullptr, 0, &l_len
       ) != FALSE) ||
      ::GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
    THROW_WIN32(::GetLastError());
  }
  wil::unique_hlocal_security_descriptor const l_sd{::LocalAlloc(LMEM_FIXED, l_len)};
  THROW_IF_WIN32_BOOL_FALSE(
      ::GetFileSecurityW(
          in_file_path.c_str(), OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION,
          l_sd.get(), l_len, &l_len
      )
  );

  wil::unique_handle l_token{};
  THROW_IF_WIN32_BOOL_FALSE(
      ::OpenProcessToken(
          ::GetCurrentProcess(), TOKEN_IMPERSONATE | TOKEN_QUERY | TOKEN_DUPLICATE | STANDARD_RIGHTS_READ,
          l_token.addressof()
      )
  );

  BOOL l_result{FALSE};
  {
    THROW_IF_WIN32_BOOL_FALSE(::ImpersonateLoggedOnUser(l_token.get()));
    auto l_scope_exit = wil::scope_exit([&]() { ::RevertToSelf(); });

    wil::unique_handle l_duplicate_token{};
    THROW_IF_WIN32_BOOL_FALSE(
        ::DuplicateToken(
            l_token.get(), ::SECURITY_IMPERSONATION_LEVEL::SecurityImpersonation, l_duplicate_token.addressof()
        )
    );
    ::GENERIC_MAPPING l_mapping{FILE_GENERIC_READ, FILE_GENERIC_WRITE, FILE_GENERIC_EXECUTE, FILE_ALL_ACCESS};
    ::DWORD l_access{DELETE};
    ::MapGenericMask(&l_access, &l_mapping);

    PRIVILEGE_SET l_privileges{0};
    DWORD l_grantedAccess = 0, l_privilegesLength = sizeof(l_privileges);
    THROW_IF_WIN32_BOOL_FALSE(
        ::AccessCheck(
            l_sd.get(), l_duplicate_token.get(), l_access, &l_mapping, &l_privileges, &l_privilegesLength,
            &l_grantedAccess, &l_result
        )
    );
  }

  return l_result == TRUE && (!FSys::is_regular_file(in_file_path) || folder_is_save(in_file_path.parent_path()));
#else
  auto l_path = FSys::is_regular_file(in_file_path) ? in_file_path.parent_path() : in_file_path;
  auto l_temp = l_path / core_set::get_set().get_uuid_str();
  try {
    FSys::fstream{l_temp, std::ios::out} << "test_file";
  } catch (const FSys::filesystem_error& e) {
    DOODLE_LOG_INFO("文件 {} 不可写入 {}", l_path.generic_string(), e.what());
    return false;
  }
  std::error_code l_error_code{};
  FSys::remove(l_temp, l_error_code);
  if (l_error_code) {
    DOODLE_LOG_INFO("文件 {} 不可删除 {}", l_path.generic_string(), l_error_code.message());
    return false;
  }
  return true;
#endif
}
void software_flag_file(const FSys::path& in_file_path, const boost::uuids::uuid& in_uuid) {
  if (!FSys::exists(in_file_path.parent_path())) return;

  auto l_path = in_file_path;
  l_path.replace_extension(doodle_config::doodle_flag_name);

  wil::unique_handle l_file_handle{::CreateFileW(
      l_path.c_str(), GENERIC_WRITE, FILE_SHARE_WRITE, nullptr, FSys::exists(l_path) ? TRUNCATE_EXISTING : CREATE_NEW,
      FILE_ATTRIBUTE_HIDDEN, nullptr
  )};
  if (!l_file_handle.is_valid()) {
    THROW_WIN32(::GetLastError());
  }
  std::string l_uuid_str = boost::uuids::to_string(in_uuid);
  THROW_IF_WIN32_BOOL_FALSE(::WriteFile(l_file_handle.get(), l_uuid_str.data(), l_uuid_str.size(), nullptr, nullptr));

  //  auto l_file_attr = ::GetFileAttributesW(l_path.c_str());
  //  if (l_file_attr == INVALID_FILE_ATTRIBUTES) {
  //    THROW_WIN32(::GetLastError());
  //  }
  //  THROW_IF_WIN32_BOOL_FALSE(::SetFileAttributesW(l_path.c_str(), l_file_attr | FILE_ATTRIBUTE_HIDDEN));
}
bool is_hidden(const FSys::path& in_file_path) {
  if (!FSys::exists(in_file_path.parent_path())) return false;
  auto l_file_info = ::GetFileAttributesW(in_file_path.c_str());
  if (l_file_info == INVALID_FILE_ATTRIBUTES) {
    std::error_code const l_error_code{static_cast<int>(::GetLastError()), std::system_category()};
    throw std::filesystem::filesystem_error{fmt::format("GetFileAttributesW {}", in_file_path), l_error_code};
  }
  return (l_file_info & FILE_ATTRIBUTE_HIDDEN) == FILE_ATTRIBUTE_HIDDEN;
}
boost::uuids::uuid software_flag_file(const FSys::path& in_file_path) {
  auto l_path = in_file_path;
  l_path.replace_extension(doodle_config::doodle_flag_name);
  if (!FSys::exists(l_path)) return boost::uuids::nil_uuid();

  boost::uuids::uuid l_uuid{boost::uuids::nil_uuid()};
  FSys::ifstream{l_path, FSys::ofstream::in | FSys::ifstream::binary} >> l_uuid;

  return l_uuid;
};

bool is_windows_remote_path(const FSys::path& in_file_path) {
  return in_file_path.has_root_path() && in_file_path.root_path().generic_string().starts_with("//");
}
FSys::path split_uuid_path(const FSys::path& in_file_path) {
  auto l_filename        = in_file_path.stem().generic_string();
  FSys::path l_uuid_str1 = l_filename.substr(0, 3);
  auto l_uuid_str2       = l_filename.substr(3, 3);
  return l_uuid_str1 / l_uuid_str2 / in_file_path;
}

bool copy_diff_impl(const FSys::path& from, const FSys::path& to, logger_ptr in_logger) {
  if (from.extension() == doodle_config::doodle_flag_name) return false;
  if (!FSys::exists(to) /* || FSys::file_size(from) != FSys::file_size(to)  */ ||
      FSys::last_write_time(from) > FSys::last_write_time(to)) {
    if (auto l_p = to.parent_path(); !FSys::exists(l_p)) FSys::create_directories(l_p);
    if (in_logger) in_logger->warn("复制 {} -> {}", from, to);
    return FSys::copy_file(from, to, FSys::copy_options::overwrite_existing);
  }
  return false;
}

bool copy_diff(const FSys::path& from, const FSys::path& to, logger_ptr in_logger) {
  if (!FSys::exists(from)) return false;
  if (in_logger) in_logger->warn("复制 {} -> {}", from, to);
  if (FSys::is_regular_file(from)) return copy_diff_impl(from, to, in_logger);
  bool l_ret{};
  for (auto&& l_file : FSys::recursive_directory_iterator(from)) {
    auto l_to_file = to / l_file.path().lexically_proximate(from);
    if (l_file.is_regular_file()) l_ret |= copy_diff_impl(l_file.path(), l_to_file, in_logger);
  }
  return l_ret;
}
bool is_diff(const FSys::path& in_path1, const FSys::path& in_path2) {
  if (!FSys::exists(in_path1) || !FSys::exists(in_path2)) return true;
  // if (FSys::file_size(in_path1) != FSys::file_size(in_path2)) return true;
  if (FSys::last_write_time(in_path1) != FSys::last_write_time(in_path2)) return true;
  return false;
}

bool is_old_file(const FSys::path& in_path1, const FSys::path& in_path2) {
  if (!FSys::exists(in_path1)) return true;
  if (!FSys::exists(in_path2)) return false;
  // if (FSys::file_size(in_path1) != FSys::file_size(in_path2)) return false;
  if (FSys::last_write_time(in_path1) < FSys::last_write_time(in_path2)) return true;
  return false;
}

}  // namespace doodle::FSys

#ifndef USE_STD_FSYS

namespace nlohmann {
// template <>
void adl_serializer<boost::filesystem::path>::to_json(json& j, const boost::filesystem::path& in_path) {
  j = in_path.generic_string();
}
void adl_serializer<boost::filesystem::path>::from_json(const json& j, boost::filesystem::path& in_path) {
  in_path = j.get<std::string>();
}
}  // namespace nlohmann

#endif