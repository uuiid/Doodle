//
// Created by TD on 25-3-18.
//

#pragma once

#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/metadata/base.h>
namespace doodle {
struct DOODLE_CORE_API studio {
  DOODLE_BASE_FIELDS();
  std::string name_;
  std::string color_;
  bool archived_;
  // to json
  friend void to_json(nlohmann::json& j, const studio& p) {
    j["id"] = p.uuid_id_;
    j["name"] = p.name_;
    j["color"] = p.color_;
    j["archived"] = p.archived_;
  }
};
}  // namespace doodle