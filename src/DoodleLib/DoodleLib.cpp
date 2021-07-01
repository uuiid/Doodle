//
// Created by TD on 2021/5/9.
//
#pragma once
#include <DoodleLib_fwd.h>
#include <Windows.h>

namespace doodle::FSys{
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
}  // namespace doodle
