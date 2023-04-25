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
#include <nlohmann/json.hpp>
#include <shellapi.h>
#include <tchar.h>

namespace doodle::FSys {

#ifdef USE_STD_FSYS
///  这个在windows上使用
///  以100纳秒为时间单位的时间段
using filetime_duration = std::chrono::duration<std::int64_t, std::ratio<1, 10'000'000>>;
/// 从公元 1601 年到公元 1970 年有 369 年的差异，转换为 11644473600秒
constexpr std::chrono::duration<std::int64_t> nt_to_unix_epoch{INT64_C(-11644473600)};
static constexpr chrono::seconds S_epoch_diff{6437664000};

std::time_t last_write_time_t(const path &in_path) {
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

chrono::sys_time_pos last_write_time_point(const path &in_path) {
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
      chrono::duration_cast<chrono::system_clock::duration>(chrono::floor<std::chrono::seconds>(withUnixEpoch))};
#elif defined(__linux__) && defined(__GNUC__)
  chrono::sys_time_pos k_sys_time_pos{k_time.time_since_epoch()};
  k_sys_time_pos -= S_epoch_diff;
  return k_sys_time_pos;
#endif
}
void last_write_time_point(const path &in_path, const std::chrono::system_clock::time_point &in_time_point) {
  const auto asDuration  = chrono::duration_cast<filetime_duration>(in_time_point.time_since_epoch());
  const auto withNtEpoch = asDuration - nt_to_unix_epoch;
  std::filesystem::file_time_type k_time{withNtEpoch};
  std::filesystem::last_write_time(in_path, k_time);
}
#else
std::time_t last_write_time_t(const path &in_path) { return last_write_time(in_path); }

chrono::sys_time_pos last_write_time_point(const path &in_path) {
  return chrono::sys_time_pos::clock::from_time_t(last_write_time(in_path));
}

void last_write_time_point(const path &in_path, const std::chrono::system_clock::time_point &in_time_point) {
  last_write_time(in_path, std::chrono::system_clock::to_time_t(in_time_point));
}

#endif

void open_explorer(const path &in_path) {
  DOODLE_LOG_INFO("打开路径: {}", in_path.generic_string());
  ShellExecute(
      nullptr, _T("open"), _T("EXPLORER.EXE"), in_path.lexically_normal().native().c_str(), nullptr, SW_SHOWDEFAULT
  );
  // std::system(fmt::format(R"(explorer.exe {})", in_path.generic_string()).c_str());
}

void backup_file(const path &source) {
  auto backup_path = source.parent_path() / "backup" / add_time_stamp(source).filename();
  if (!exists(backup_path.parent_path())) create_directories(backup_path.parent_path());
  rename(source, backup_path);
}
path add_time_stamp(const path &in_path) {
  auto k_fn = in_path.stem();
  k_fn += date::format("_%y_%m_%d_%H_%M_%S_", std::chrono::system_clock::now());
  k_fn += in_path.extension();
  auto k_path = in_path.parent_path() / k_fn;
  return k_path;
}

std::vector<path> list_files(const path &in_dir) {
  return std::vector<path>{directory_iterator{in_dir}, directory_iterator{}};
}
bool is_sub_path(const path &in_parent, const path &in_child) {
  return boost::istarts_with(in_parent.generic_string(), in_child.generic_string());
}

FSys::path write_tmp_file(
    const std::string &in_falg, const std::string &in_string, const std::string &in_extension,
    const std::optional<std::string> &in_file_name, std::int32_t in_model
) {
  auto tmp_path = core_set::get_set().get_cache_root(fmt::format(
      "{}/v{}{}{}", in_falg, version::build_info::get().version_major, version::build_info::get().version_minor,
      version::build_info::get().version_patch
  ));
  auto k_tmp_path =
      tmp_path / FSys::path{
                     in_file_name ? (*in_file_name + in_extension)
                                  : boost::uuids::to_string(core_set::get_set().get_uuid()) + in_extension};
  if (in_file_name && FSys::exists(k_tmp_path)) {
    return k_tmp_path;
  } else {
    // 写入文件后直接关闭
    FSys::fstream file{k_tmp_path, in_model};
    file << in_string;
  }

  return k_tmp_path;
}

}  // namespace doodle::FSys

#ifndef USE_STD_FSYS

namespace nlohmann {
// template <>
void adl_serializer<boost::filesystem::path>::to_json(json &j, const boost::filesystem::path &in_path) {
  j = in_path.generic_string();
}
void adl_serializer<boost::filesystem::path>::from_json(const json &j, boost::filesystem::path &in_path) {
  in_path = j.get<std::string>();
}
}  // namespace nlohmann

#endif