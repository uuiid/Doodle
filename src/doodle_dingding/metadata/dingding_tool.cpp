//
// Created by TD on 2022/9/19.
//

#include "dingding_tool.h"
#include <doodle_core/metadata/time_point_wrap.h>
#include <nlohmann/json.hpp>

#include <date/tz.h>

#include <fmt/chrono.h>

namespace doodle::dingding::detail {
time_point_wrap tool::parse_dingding_time(const nlohmann::json& time_obj) {
  chrono::local_seconds time;
  std::istringstream l_time{time_obj.get<std::string>()};
  l_time >> chrono::parse("%Y-%m-%d %H:%M:%S", time);
  return time_point_wrap{time};
}
time_point_wrap tool::parse_dingding_Date(const nlohmann::json& time_obj) {
  chrono::milliseconds l_time{time_obj.get<chrono::milliseconds::rep>()};
  return time_point_wrap{chrono::sys_time<chrono::milliseconds>{l_time}};
}
std::string tool::print_dingding_time(const time_point_wrap& in_time) {
  return fmt::format("{:L%Y-%m-%d %H:%M:%S}", in_time);
}

}  // namespace doodle::dingding::detail
