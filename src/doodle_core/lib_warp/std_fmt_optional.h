//
// Created by TD on 2022/9/16.
//

#pragma once

#include <fmt/format.h>

#include <optional>

namespace fmt {

template <typename T>
struct formatter<
    ::std::optional<T>> : formatter<std::string> {
  template <typename FormatContext>
  auto format(
      const ::std::optional<T> &in_,
      FormatContext &ctx
  ) const -> decltype(ctx.out()) {
    return fmt::format_to(
        ctx.out(),
        "{}",
        in_ ? fmt::to_string(*in_) : std::string{}
    );
  }
};

}  // namespace fmt
