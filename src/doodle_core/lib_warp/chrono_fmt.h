//
// Created by TD on 2021/9/17.
//

#pragma once

#include <boost/hana.hpp>

#include <chrono>
#include <fmt/format.h>

namespace fmt {
template <typename Char, typename Duration>
struct formatter<::std::chrono::time_point<::std::chrono::file_clock, Duration>, Char>
    : formatter<::std::chrono::time_point<::std::chrono::system_clock, Duration>, Char> {
  using time_duration_type = ::std::chrono::time_point<::std::chrono::local_t, Duration>;
  using base_type          = formatter<::std::chrono::time_point<::std::chrono::system_clock, Duration>, Char>;

  template <typename FormatContext>
  auto format(const time_duration_type& in_, FormatContext& ctx) const -> decltype(ctx.out()) {
    return base_type::format(std::chrono::clock_cast<::std::chrono::system_clock>(in_), ctx);
  }
};
template <typename Char, typename Duration>
struct formatter<::std::chrono::zoned_time<Duration, const std::chrono::time_zone*>, Char>
    : formatter<::std::chrono::time_point<::std::chrono::system_clock, Duration>, Char> {
  using zoned_time_type = ::std::chrono::zoned_time<Duration, const std::chrono::time_zone*>;
  using base_type       = formatter<::std::chrono::time_point<::std::chrono::system_clock, Duration>, Char>;

  template <typename FormatContext>
  auto format(const zoned_time_type& in_, FormatContext& ctx) const -> decltype(ctx.out()) {
    return base_type::format(in_.get_sys_time(), ctx);
  }
};

}  // namespace fmt