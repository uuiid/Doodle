//
// Created by TD on 2022/9/19.
//

#pragma once

#include <doodle_dingding/doodle_dingding_fwd.h>
#include <nlohmann/json_fwd.hpp>
namespace doodle {
class time_point_wrap;
}

namespace doodle::dingding::detail {

class DOODLE_DINGDING_API tool {
 public:
  static time_point_wrap parse_dingding_time(const nlohmann::json& time_obj);
  static time_point_wrap parse_dingding_Date(const nlohmann::json& time_obj);
  static std::string print_dingding_time(const time_point_wrap& in_time);
};

}  // namespace doodle::dingding::detail
