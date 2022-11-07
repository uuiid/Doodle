//
// Created by TD on 2022/3/31.
//

#pragma once

#include <cstdint>
#include <string>
#include <chrono>
#include <doodle_core/configure/doodle_core_export.h>
namespace doodle::version {

class DOODLE_CORE_API build_info {
  build_info();

 public:
  virtual ~build_info() = default;

  static build_info& get();

  build_info(const build_info&) noexcept            = delete;
  build_info(build_info&&) noexcept                 = delete;
  build_info& operator=(const build_info&) noexcept = delete;
  build_info& operator=(build_info&&) noexcept      = delete;
  std::int16_t version_major;
  std::int16_t version_minor;
  std::int16_t version_patch;
  std::int16_t version_tweak;
  std::string version_str;
  std::string build_time;
};

}  // namespace doodle::version

#if defined _WIN32

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#else
#undef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#else
#undef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif

#ifndef _SILENCE_CXX17_ITERATOR_BASE_CLASS_DEPRECATION_WARNING
#define _SILENCE_CXX17_ITERATOR_BASE_CLASS_DEPRECATION_WARNING
#else
#undef _SILENCE_CXX17_ITERATOR_BASE_CLASS_DEPRECATION_WARNING
#define _SILENCE_CXX17_ITERATOR_BASE_CLASS_DEPRECATION_WARNING
#endif

#ifndef _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING
#define _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING
#else
#undef _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING
#define _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING
#endif

#elif defined __linux__  // _WIN32

#endif  // __linux__

#include <boost/current_function.hpp>
#ifndef SPDLOG_FUNCTION
#define SPDLOG_FUNCTION static_cast<const char*>(BOOST_CURRENT_FUNCTION)
#else
#undef SPDLOG_FUNCTION
#define SPDLOG_FUNCTION static_cast<const char*>(BOOST_CURRENT_FUNCTION)
#endif
