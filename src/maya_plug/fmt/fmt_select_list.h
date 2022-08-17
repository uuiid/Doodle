//
// Created by TD on 2022/5/11.
//
#pragma once

#include <maya/MSelectionList.h>

#include <fmt/format.h>
#include <fmt/ostream.h>

#include <maya/MString.h>
#include <maya/MStringArray.h>

namespace fmt {
/**
 * @brief 格式化maya字符串
 *
 * @tparam
 */
template <>
struct formatter<MSelectionList> : formatter<string_view> {
  template <typename FormatContext>
  auto format(const MSelectionList& in_, FormatContext& ctx) const -> decltype(ctx.out()) {
    MStringArray l_array{};
    in_.getSelectionStrings(l_array);
    return formatter<string_view>::format(
        to_string(l_array),
        ctx);
  }
};
}  // namespace fmt
