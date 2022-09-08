//
// Created by TD on 2022/8/8.
//

#include "user_set_data.h"
#include <doodle_core/metadata/metadata.h>

namespace doodle::detail {
void to_json(nlohmann::json& j, const user_set_data& p) {
  j["rules_attr"] = p.rules_attr;
  j["data_ref"]   = p.data_ref;
  j["user_data"]  = p.user_data;
}
void from_json(const nlohmann::json& j, user_set_data& p) {
  j.at("rules_attr").get_to(p.rules_attr);
  j.at("data_ref").get_to(p.data_ref);
  j.at("user_data").get_to(p.user_data);
}
}  // namespace doodle
