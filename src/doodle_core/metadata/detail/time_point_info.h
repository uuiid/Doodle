//
// Created by TD on 2022/8/4.
//

#pragma once

#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/metadata/time_point_wrap.h>

namespace doodle {
namespace business {
namespace rules_ns {
class time_point_info;
void to_json(nlohmann::json& j, const time_point_info& p);
void from_json(const nlohmann::json& j, time_point_info& p);
class time_point_info {
 public:
  time_point_info() = default;
  time_point_info(time_point_wrap in_b, time_point_wrap in_e, std::string in_indo)
      : first(in_b),
        second(in_e),
        info(std::move(in_indo)) {}

  time_point_info(time_point_wrap in_b, time_point_wrap in_e)
      : time_point_info(in_b, in_e, ""s) {}

  time_point_wrap first{};
  time_point_wrap second{};
  std::string info{};

  friend void to_json(nlohmann::json& j, const time_point_info& p);
  friend void from_json(const nlohmann::json& j, time_point_info& p);
};

}  // namespace rules_ns
}  // namespace business
}  // namespace doodle

namespace fmt {
/**
 * @brief 集数格式化程序
 *
 * @tparam
 */
template <>
struct formatter<::doodle::business::rules_ns::time_point_info> : formatter<std::string> {
  template <typename FormatContext>
  auto format(const ::doodle::business::rules_ns::time_point_info& in_, FormatContext& ctx) -> decltype(ctx.out()) {
    return format_to(ctx.out(), "{} {} {}", in_.info, in_.first, in_.second);
  }
};
}  // namespace fmt
