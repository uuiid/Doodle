//
// Created by TD on 2022/8/29.
//

#pragma once
#include <boost/system.hpp>

#include <fmt/format.h>
#include <fmt/ostream.h>

namespace fmt {
template <>
struct formatter<::boost::system::error_code> : formatter<string_view> {
  template <typename FormatContext>
  auto format(const ::boost::system::error_code& in_, FormatContext& ctx) const -> decltype(ctx.out()) {
    auto l_str = in_.message() + '[' + in_.to_string() + ']';
    return formatter<string_view>::format(l_str, ctx);
  }
};
}  // namespace fmt
