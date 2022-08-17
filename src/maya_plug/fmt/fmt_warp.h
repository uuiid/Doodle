//
// Created by TD on 2021/12/23.
//

#pragma once

#include <fmt/format.h>

#include <maya/MString.h>

namespace fmt {
/**
 * @brief 格式化maya字符串
 *
 * @tparam
 */
template <>
struct formatter<MString> : formatter<string_view> {
  template <typename FormatContext>
  auto format(const MString& in_, FormatContext& ctx) const -> decltype(ctx.out()) {
    std::string k_str = in_.asUTF8();
    return formatter<string_view>::format(
        k_str,
        ctx);
  }
};
}  // namespace fmt
