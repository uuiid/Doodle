#pragma once

#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/metadata/base.h>

#include <string>

namespace doodle::seedance2 {
struct DOODLE_CORE_API assets_entity {
  DOODLE_BASE_FIELDS();
  std::string name_;
  std::string description_;
  uuid group_id_;
  uuid user_id_;
  uuid preview_id_;
  uuid ai_studio_id_;

  chrono::system_zoned_time created_at_{chrono::current_zone(), chrono::system_clock::now()};
  chrono::system_zoned_time updated_at_{chrono::current_zone(), chrono::system_clock::now()};
  // to json
  friend void to_json(nlohmann::json& j, const assets_entity& p) {
    j["id"]           = p.uuid_id_;
    j["name"]         = p.name_;
    j["description"]  = p.description_;
    j["group_id"]     = p.group_id_;
    j["user_id"]      = p.user_id_;
    j["preview_id"]   = p.preview_id_;
    j["ai_studio_id"] = p.ai_studio_id_;
    j["created_at"]   = p.created_at_;
    j["updated_at"]   = p.updated_at_;
  }
  // from json
  friend void from_json(const nlohmann::json& j, assets_entity& p) {
    if (j.contains("name")) j.at("name").get_to(p.name_);
    if (j.contains("description")) j.at("description").get_to(p.description_);
    if (j.contains("group_id")) j.at("group_id").get_to(p.group_id_);
    if (j.contains("preview_id")) j.at("preview_id").get_to(p.preview_id_);
  }
};
}  // namespace doodle::seedance2