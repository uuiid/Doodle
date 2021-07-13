﻿//
// Created by TD on 2021/5/9.
//
#pragma once
#include <DoodleLib_fwd.h>
#include <Exception/Exception.h>
#include <Windows.h>
#include <shellapi.h>

namespace doodle::FSys {
std::time_t last_write_time_t(const path &in_path) {
  auto k_h = CreateFile(in_path.generic_wstring().c_str(), 0, 0, nullptr, OPEN_EXISTING, 0, nullptr);
  if (k_h == INVALID_HANDLE_VALUE)
    throw std::runtime_error{"无效的文件路径"};
  FILETIME k_f_l;
  if (!GetFileTime(k_h, nullptr, nullptr, &k_f_l)) {
    throw std::runtime_error{"无法获得写入时间"};
  }
  ULARGE_INTEGER ull{};
  ull.LowPart  = k_f_l.dwLowDateTime;
  ull.HighPart = k_f_l.dwHighDateTime;

  return static_cast<time_t>(ull.QuadPart / 10000000ULL - 11644473600ULL);
}

void open_explorer(const path &in_path) {
  DOODLE_LOG_INFO("打开路径: {}", in_path.generic_string());
  ShellExecute(nullptr, (L"open"), in_path.generic_wstring().c_str(), nullptr, nullptr, SW_SHOWDEFAULT);
  // std::system(fmt::format(R"(explorer.exe {})", in_path.generic_string()).c_str());
}
void backup_file(const path &source) {
  auto backup_path = source.parent_path() / "backup" / add_time_stamp(source).filename();
  if (!exists(backup_path.parent_path()))
    create_directories(backup_path.parent_path());
  rename(source, backup_path);
  if (exists(source))
    throw DoodleError{"无法备份文件"};
}
path add_time_stamp(const path &in_path) {
  auto k_fn = in_path.stem();
  k_fn += date::format("_%m_%d_%y_%H_%M_%S_", std::chrono::system_clock::now());
  k_fn += in_path.extension();
  auto k_path = in_path.parent_path() / k_fn;
  return k_path;
}

}  // namespace doodle::FSys
