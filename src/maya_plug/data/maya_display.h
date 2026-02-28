//
// Created by TD on 2021/12/16.
//

#pragma once

#include <maya_plug/data/maya_conv_str.h>
#include <maya_plug/fmt/fmt_warp.h>

#include <maya/MGlobal.h>

namespace doodle::maya_plug {

template <typename... Args>
void display_warning(fmt::format_string<Args...> fmt_str, Args&&... in_args) {
  MGlobal::displayWarning(
      conv::to_ms(fmt::format(std::forward<fmt::format_string<Args...>>(fmt_str), std::forward<Args>(in_args)...))
  );
}
inline void display_warning(const std::string& in_str) { MGlobal::displayWarning(conv::to_ms(in_str)); }

template <typename... Args>
void display_info(fmt::format_string<Args...> fmt_str, Args&&... in_args) {
  MGlobal::displayInfo(
      conv::to_ms(fmt::format(std::forward<fmt::format_string<Args...>>(fmt_str), std::forward<Args>(in_args)...))
  );
}
inline void display_info(const std::string& in_str) { MGlobal::displayInfo(conv::to_ms(in_str)); }

template <typename... Args>
void display_error(fmt::format_string<Args...> fmt_str, Args&&... in_args) {
  MGlobal::displayError(
      conv::to_ms(fmt::format(std::forward<fmt::format_string<Args...>>(fmt_str), std::forward<Args>(in_args)...))
  );
}
inline void display_error(const std::string& in_str) { MGlobal::displayError(conv::to_ms(in_str)); }

}  // namespace doodle::maya_plug
