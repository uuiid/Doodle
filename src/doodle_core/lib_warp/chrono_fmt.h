//
// Created by TD on 2021/9/17.
//

#pragma once

#include <boost/hana.hpp>

#include "date/date.h"
#include "date/tz.h"
#include <chrono>
#include <fmt/format.h>

namespace fmt {
template <typename Char, typename Duration>
struct formatter<::std::chrono::time_point<::date::local_t, Duration>, Char>
    : formatter<::std::chrono::time_point<::std::chrono::system_clock, Duration>, Char> {
  using time_duration_type = ::std::chrono::time_point<::date::local_t, Duration>;
  using base_type          = formatter<::std::chrono::time_point<::std::chrono::system_clock, Duration>, Char>;

  template <typename FormatContext>
  auto format(const time_duration_type& in_, FormatContext& ctx) const -> decltype(ctx.out()) {
    return base_type::format(date::clock_cast<::std::chrono::system_clock>(in_), ctx);
  }
};
}  // namespace fmt