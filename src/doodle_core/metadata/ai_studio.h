//
// Created by TD on 25-3-18.
//

#pragma once

#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/metadata/base.h>
namespace doodle {
struct DOODLE_CORE_API ai_studio {
  DOODLE_BASE_FIELDS();
  std::string name_;
  std::string color_;
  std::string app_key_;
  std::string app_secret_;
  bool archived_;
  // to json
  friend void to_json(nlohmann::json& j, const ai_studio& p) {
    j["id"]       = p.uuid_id_;
    j["name"]     = p.name_;
    j["color"]    = p.color_;
    j["archived"] = p.archived_;
  }
  // from json
  friend void from_json(const nlohmann::json& j, ai_studio& p) {
    if (j.contains("name")) j.at("name").get_to(p.name_);
    if (j.contains("color")) j.at("color").get_to(p.color_);
    if (j.contains("archived")) j.at("archived").get_to(p.archived_);
    if (j.contains("app_key")) j.at("app_key").get_to(p.app_key_);
    if (j.contains("app_secret")) j.at("app_secret").get_to(p.app_secret_);
  }
};

struct ai_studio_person_role_link {
  std::int64_t id_;
  uuid ai_studio_id_;
  uuid person_id_;

  // to json
  friend void to_json(nlohmann::json& j, const ai_studio_person_role_link& p) {
    j["ai_studio_id"] = p.ai_studio_id_;
    j["person_id"]    = p.person_id_;
  }
  // from json
  friend void from_json(const nlohmann::json& j, ai_studio_person_role_link& p) {
    if (j.contains("ai_studio_id")) j.at("ai_studio_id").get_to(p.ai_studio_id_);
    if (j.contains("person_id")) j.at("person_id").get_to(p.person_id_);
  }
};

}  // namespace doodle