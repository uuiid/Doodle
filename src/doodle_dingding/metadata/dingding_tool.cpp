//
// Created by TD on 2022/9/19.
//

#include "dingding_tool.h"
#include <doodle_core/metadata/time_point_wrap.h>
#include <nlohmann/json.hpp>
namespace doodle::dingding::detail {
time_point_wrap tool::parse_dingding_time(const nlohmann::json& time_obj) {
  chrono::local_seconds time;
  std::istringstream l_time{time_obj.get<std::string>()};
  l_time >> chrono::parse("%Y-%m-%d %H:%M:%S", time);
  return time_point_wrap{time};
}
time_point_wrap tool::parse_dingding_Date(const nlohmann::json& time_obj) {
  chrono::microseconds l_time{time_obj.get<chrono::microseconds::rep>()};
  return time_point_wrap{chrono::sys_time<chrono::microseconds>{l_time}};
}

}  // namespace doodle::dingding::detail
