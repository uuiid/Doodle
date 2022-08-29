//
// Created by TD on 2022/8/29.
//

#pragma once
#include <fmt/format.h>
#include <fmt/ostream.h>

#include <boost/system.hpp>

namespace fmt {
template <>
struct formatter<::boost::system::error_code> : formatter<string_view> {
  template <typename FormatContext>
  auto format(const ::boost::system::error_code& in_, FormatContext& ctx) const -> decltype(ctx.out()) {
    return formatter<string_view>::format(
        in_.to_string(),
        ctx
    );
  }
};
}
