//
// Created by TD on 2022/9/19.
//

#pragma once

#include <fmt/format.h>
#include <boost/system.hpp>
#include <boost/exception/diagnostic_information.hpp>
namespace fmt {
/**
 * @brief 格式化库异常
 *
 * @tparam
 */
template <>
struct formatter<::std::runtime_error> : formatter<string_view> {
  template <typename FormatContext>
  auto format(const ::std::runtime_error& in_, FormatContext& ctx) const -> decltype(ctx.out()) {
    return formatter<string_view>::format(
        boost::diagnostic_information(in_),
        ctx
    );
  }
};
}  // namespace fmt
