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
    auto& l_task_json        = j["tasks"];
    for (const auto& task : tasks_) {
      nlohmann::json l_j_task{};
      l_j_task["id"]                   = task.uuid_id_;
      l_j_task["due_date"]             = task.due_date_;
      l_j_task["done_date"]            = task.done_date_;
      l_j_task["duration"]             = task.duration_;
      l_j_task["entity_id"]            = task.entity_id_;
      l_j_task["estimation"]           = task.estimation_;
      l_j_task["end_date"]             = task.end_date_;
      l_j_task["is_subscribed"]        = false;
      l_j_task["last_comment_date"]    = task.last_comment_date_;
      l_j_task["last_preview_file_id"] = task.last_preview_file_id_;
      l_j_task["priority"]             = task.priority_;
      l_j_task["real_start_date"]      = task.real_start_date_;
      l_j_task["retake_count"]         = task.retake_count_;
      l_j_task["start_date"]           = task.start_date_;
      l_j_task["difficulty"]           = task.difficulty_;
      l_j_task["task_status_id"]       = task.task_status_id_;
      l_j_task["task_type_id"]         = task.task_type_id_;
      l_j_task["assignees"]            = task.assignees_;
      l_task_json.emplace_back(std::move(l_j_task));
    }
  }
};
}  // namespace doodle
