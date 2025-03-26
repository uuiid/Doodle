//
// Created by TD on 25-3-25.
//

#pragma once
#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/metadata/entity.h>
#include <doodle_core/metadata/task.h>

namespace doodle {

struct DOODLE_CORE_API entity_task_t : entity {
  std::vector<task> tasks_;
  std::shared_ptr<asset_type> asset_type_;
  // to json
  void to_json(nlohmann::json& j) const {
    j["id"]                  = uuid_id_;
    j["name"]                = name_;
    j["preview_file_id"]     = preview_file_id_;
    j["description"]         = description_;
    j["asset_type_name"]     = asset_type_ ? asset_type_->name_ : std::string{};
    j["asset_type_id"]       = asset_type_ ? asset_type_->uuid_id_ : uuid{};
    j["canceled"]            = canceled_;
    j["ready_for"]           = ready_for_;
    j["episode_id"]          = source_id_;
    j["casting_episode_ids"] = nlohmann::json::array();
    j["is_casting_standby"]  = is_casting_standby_;
    j["is_shared"]           = is_shared_;
    j["data"]                = data_;
    j["tasks"]               = nlohmann::json::array();
  }
};
}  // namespace doodle
