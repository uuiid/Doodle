#pragma once

#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/metadata/base.h>

#include <string>

namespace doodle::seedance2 {
struct DOODLE_CORE_API subproject {
  DOODLE_BASE_FIELDS();
  std::string name_;
  uuid project_id_;
  uuid created_user_id_;

  bool archived_{false};
  chrono::system_zoned_time created_at_{chrono::current_zone(), chrono::system_clock::now()};

  // to json
  friend void to_json(nlohmann::json& j, const subproject& p) {
    j["id"]              = p.uuid_id_;
    j["name"]            = p.name_;
    j["project_id"]      = p.project_id_;
    j["created_user_id"] = p.created_user_id_;
    j["created_at"]      = p.created_at_;
    j["archived"]        = p.archived_;
  }
  // from json
  friend void from_json(const nlohmann::json& j, subproject& p) {
    j.at("name").get_to(p.name_);
    j.at("project_id").get_to(p.project_id_);
  }
};

// 子项目对应的参与人员
struct DOODLE_CORE_API subproject_person_link {
  DOODLE_BASE_FIELDS();
  uuid subproject_id_;
  uuid person_id_;
  // to json
  friend void to_json(nlohmann::json& j, const subproject_person_link& p) {
    j["id"]            = p.uuid_id_;
    j["subproject_id"] = p.subproject_id_;
    j["person_id"]     = p.person_id_;
  }
  // from json
  friend void from_json(const nlohmann::json& j, subproject_person_link& p) {
    j.at("subproject_id").get_to(p.subproject_id_);
    j.at("person_id").get_to(p.person_id_);
  }
};

}  // namespace doodle::seedance2