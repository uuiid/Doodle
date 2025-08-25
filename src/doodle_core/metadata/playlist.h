//
// Created by TD on 25-8-25.
//

#pragma once
#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/metadata/base.h>
namespace doodle {
struct DOODLE_CORE_API playlist {
  DOODLE_BASE_FIELDS();
  std::string name_;
  nlohmann::json shots_;

  uuid project_id_;
  uuid episodes_id_;
  uuid task_type_id_;
  bool for_client_;
  bool for_entity_;
  bool is_for_all_;

  chrono::system_zoned_time created_at_;
  chrono::system_zoned_time updated_at_;
  // to json
  friend void to_json(nlohmann::json& j, const playlist& p) {
    j["id"]           = p.uuid_id_;
    j["name"]         = p.name_;
    j["shots"]        = p.shots_;
    j["project_id"]   = p.project_id_;
    j["episodes_id"]  = p.episodes_id_;
    j["task_type_id"] = p.task_type_id_;
    j["for_client"]   = p.for_client_;
    j["is_for_all"]   = p.is_for_all_;
    j["created_at"]   = p.created_at_;
    j["updated_at"]   = p.updated_at_;
  }
};
}  // namespace doodle