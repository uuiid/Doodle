//
// Created by TD on 25-3-11.
//

#pragma once

#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/metadata/base.h>

#include <chrono>
#include <string>

namespace doodle {
struct updata_logs {
  DOODLE_BASE_FIELDS();
  std::string log_;

  chrono::system_zoned_time created_at_{chrono::current_zone(), chrono::system_clock::now()};
  chrono::system_zoned_time updated_at_{chrono::current_zone(), chrono::system_clock::now()};

  // to json
  friend void to_json(nlohmann::json& j, const updata_logs& p) {
    j["id"]         = p.uuid_id_;
    j["log"]        = p.log_;
    j["created_at"] = p.created_at_;
    j["updated_at"] = p.updated_at_;
  }
};
}  // namespace doodle