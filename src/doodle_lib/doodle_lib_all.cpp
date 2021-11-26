//
// Created by TD on 2021/5/9.
//

#include <Exception/exception.h>
#include <cryptopp/base64.h>
#include <cryptopp/files.h>
#include <cryptopp/hex.h>
#include <cryptopp/sha.h>
#include <doodle_lib_fwd.h>

#include <boost/locale.hpp>

#ifdef _WIN32
#include <Windows.h>
#include <shellapi.h>
#include <tchar.h>
#elif defined(__linux__)

#endif

namespace doodle {

namespace chrono {
bool is_rest_day(const sys_days &in_days) {
  weekday k_weekday{in_days};
  return k_weekday == Sunday || k_weekday == Saturday;
}
}  // namespace chrono

namespace FSys {
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
  return chrono::system_clock::time_point{chrono::duration_cast<chrono::system_clock::duration>(withUnixEpoch)};
#elif defined(__linux__) && defined(__GNUC__)
  chrono::sys_time_pos k_sys_time_pos{k_time.time_since_epoch()};
  k_sys_time_pos -= S_epoch_diff;
  return k_sys_time_pos;
#endif
}
void last_write_time_point(const path &in_path, const std::chrono::system_clock::time_point &in_time_point) {
  const auto asDuration = chrono::duration_cast<filetime_duration>(
      in_time_point.time_since_epoch());
  const auto withNtEpoch = asDuration - nt_to_unix_epoch;
  std::filesystem::file_time_type  k_time{withNtEpoch};
  std::filesystem::last_write_time(in_path,k_time);
}
void open_explorer(const path &in_path) {
#if defined(_WIN32)
  DOODLE_LOG_INFO("打开路径: {}", in_path.generic_string());
  ShellExecute(nullptr, _T("open"), in_path.generic_wstring().c_str(), nullptr, nullptr, SW_SHOWDEFAULT);
  // std::system(fmt::format(R"(explorer.exe {})", in_path.generic_string()).c_str());
#else
  throw DoodleError{"没有这个函数"};
#endif
}
void backup_file(const path &source) {
  auto backup_path = source.parent_path() / "backup" / add_time_stamp(source).filename();
  if (!exists(backup_path.parent_path()))
    create_directories(backup_path.parent_path());
  rename(source, backup_path);
  if (exists(source))
    throw doodle_error{"无法备份文件"};
}
path add_time_stamp(const path &in_path) {
  auto k_fn = in_path.stem();
  k_fn += date::format("_%y_%m_%d_%H_%M_%S_", std::chrono::system_clock::now());
  k_fn += in_path.extension();
  auto k_path = in_path.parent_path() / k_fn;
  return k_path;
}
std::string file_hash_sha224(const path &in_file) {
  if (exists(in_file) && is_regular_file(in_file)) {
    CryptoPP::SHA224 k_sha_224;
    std::string k_string;
    ifstream k_ifstream{in_file, std::ios::binary | std::ios::in};
    if (!k_ifstream) {
      DOODLE_LOG_INFO("{} 无法打开", in_file)
      throw filesystem_error{fmt::format("{} 无法打开", in_file), in_file,
                             std::make_error_code(std::errc::bad_file_descriptor)};
    }
    CryptoPP::FileSource k_file{
        k_ifstream,
        true,
        new CryptoPP::HashFilter{
            k_sha_224,
            new CryptoPP::HexEncoder{
                new CryptoPP::StringSink{k_string}}}};
    return k_string;
  } else {
    DOODLE_LOG_INFO("{} 文件不存在或者不是文件", in_file)
    throw filesystem_error{fmt::format("{} 文件不存在或者不是文件", in_file), in_file,
                           std::make_error_code(std::errc::no_such_file_or_directory)};
  }
}
std::vector<path> list_files(const path &in_dir) {
  return std::vector<path>{
      directory_iterator{in_dir},
      directory_iterator{}};
}
}  // namespace FSys
}  // namespace doodle

// namespace doodle::FSys
