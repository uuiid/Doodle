//
// Created by TD on 2021/9/17.
//

#pragma once

#include <fmt/format.h>
#include <boost/hana.hpp>
#include <memory>

namespace fmt {
/**
 * @brief 格式化::std::filesystem::path
 *
 * @tparam  ::std::filesystem::path
 */
template <>
struct formatter<::std::filesystem::path> : formatter<string_view> {
  template <typename FormatContext>
  auto format(const ::std::filesystem::path &in_, FormatContext &ctx) const -> decltype(ctx.out()) {
    return formatter<fmt::string_view>::format(
        in_.generic_string(),
        ctx
    );
  }
};
}  // namespace fmt
