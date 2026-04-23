#pragma once

#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/metadata/base.h>

#include <string>

namespace doodle::seedance2 {
struct DOODLE_CORE_API assets_group {
  DOODLE_BASE_FIELDS();
  std::string label_;
  uuid user_id_;
  uuid ai_studio_id_;
  chrono::system_zoned_time created_at_{chrono::current_zone(), chrono::system_clock::now()};

  // to json
  friend void to_json(nlohmann::json& j, const assets_group& p) {
    j["id"]         = p.uuid_id_;
    j["label"]      = p.label_;
    j["user_id"]    = p.user_id_;
    j["ai_studio_id"]  = p.ai_studio_id_;
    j["created_at"] = p.created_at_;
  }
  // from json
  friend void from_json(const nlohmann::json& j, assets_group& p) {
    if (j.contains("label")) j.at("label").get_to(p.label_);
  }
};

}  // namespace doodle::seedance2