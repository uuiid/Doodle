//
// Created by TD on 2022/9/11.
//

#pragma once
#include <fmt/format.h>
#include <fmt/ostream.h>
#include <maya_plug/data/maya_tool.h>

namespace fmt{
/**
 * @brief 格式化 maya 路径
 */
template <>
struct formatter<MDagPath> : formatter<string_view> {
  template <typename FormatContext>
  auto format(const MDagPath& in_, FormatContext& ctx) const -> decltype(ctx.out()) {
    std::string k_str = ::doodle::maya_plug::get_node_name(in_);
    return formatter<string_view>::format(
        k_str,
        ctx
    );
  }
};

}
