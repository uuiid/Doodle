//
// Created by TD on 2022/8/4.
//

#include "time_point_info.h"

namespace doodle {
namespace business {
namespace rules_ns {
void to_json(nlohmann::json& j, const time_point_info& p) {
  j["first"]  = p.first;
  j["second"] = p.second;
  j["info"]   = p.info;
}
void from_json(const nlohmann::json& j, time_point_info& p) {
  j["first"].get_to(p.first);
  j["second"].get_to(p.second);
  j["info"].get_to(p.info);
}
}  // namespace rules_ns
}  // namespace business
}  // namespace doodle
