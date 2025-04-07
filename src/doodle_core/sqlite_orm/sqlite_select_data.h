//
// Created by TD on 25-3-25.
//

#pragma once
#include "doodle_core/metadata/task_status.h"
#include "doodle_core/metadata/task_type.h"
#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/metadata/entity.h>
#include <doodle_core/metadata/entity_type.h>
#include <doodle_core/metadata/task.h>

#include <sqlite_orm/sqlite_orm.h>
namespace doodle {

struct DOODLE_CORE_API entity_task_t : entity {
  struct task_impl : task {
    bool is_subscribed_;
  };

  std::vector<task_impl> tasks_;
  std::shared_ptr<asset_type> asset_type_;
  // to json
  friend void to_json(nlohmann::json& j, const entity_task_t& p) {
    j["id"]                  = p.uuid_id_;
    j["name"]                = p.name_;
    j["preview_file_id"]     = p.preview_file_id_;
    j["description"]         = p.description_;
    j["asset_type_name"]     = p.asset_type_ ? p.asset_type_->name_ : std::string{};
    j["asset_type_id"]       = p.asset_type_ ? p.asset_type_->uuid_id_ : uuid{};
    j["canceled"]            = p.canceled_;
    j["ready_for"]           = p.ready_for_;
    j["episode_id"]          = p.source_id_;
    j["casting_episode_ids"] = nlohmann::json::array();
    j["is_casting_standby"]  = p.is_casting_standby_;
    j["is_shared"]           = p.is_shared_;
    j["data"]                = p.data_;
    j["tasks"]               = nlohmann::json::array();
    auto& l_task_json        = j["tasks"];
    for (const auto& task : p.tasks_) {
      nlohmann::json l_j_task{};
      l_j_task["id"]                   = task.uuid_id_;
      l_j_task["due_date"]             = task.due_date_;
      l_j_task["done_date"]            = task.done_date_;
      l_j_task["duration"]             = task.duration_;
      l_j_task["entity_id"]            = task.entity_id_;
      l_j_task["estimation"]           = task.estimation_;
      l_j_task["end_date"]             = task.end_date_;
      l_j_task["is_subscribed"]        = task.is_subscribed_;
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
      l_j_task["path"]                 = "";
      l_task_json.emplace_back(std::move(l_j_task));
    }
  }
};

struct todo_t {
  // std::shared_ptr<project> project_;
  // std::shared_ptr<entity> entity_;
  decltype(task::uuid_id_) uuid_id_;
  decltype(task::name_) name_;
  decltype(task::description_) description_;
  decltype(task::priority_) priority_;
  decltype(task::difficulty_) difficulty_;
  decltype(task::duration_) duration_;
  decltype(task::estimation_) estimation_;
  decltype(task::completion_rate_) completion_rate_;
  decltype(task::retake_count_) retake_count_;
  decltype(task::sort_order_) sort_order_;
  decltype(task::start_date_) start_date_;
  decltype(task::due_date_) due_date_;
  decltype(task::real_start_date_) real_start_date_;
  decltype(task::end_date_) end_date_;
  decltype(task::done_date_) done_date_;
  decltype(task::last_comment_date_) last_comment_date_;
  decltype(task::nb_assets_ready_) nb_assets_ready_;
  decltype(task::data_) data_;
  decltype(task::shotgun_id_) shotgun_id_;
  decltype(task::last_preview_file_id_) last_preview_file_id_;
  decltype(task::project_id_) project_id_;
  decltype(task::task_type_id_) task_type_id_;
  decltype(task::task_status_id_) task_status_id_;
  decltype(task::assigner_id_) assigner_id_;
  decltype(task::created_at_) created_at_;
  decltype(task::updated_at_) updated_at_;

  decltype(project::name_) project_name_;
  decltype(project::has_avatar_) project_has_avatar_;

  decltype(entity::uuid_id_) entity_uuid_id_;
  decltype(entity::name_) entity_name_;
  decltype(entity::description_) entity_description_;
  decltype(entity::data_) entity_data_;
  decltype(entity::preview_file_id_) entity_preview_file_id_;

  decltype(asset_type::name_) asset_type_name_;
  decltype(entity::canceled_) entity_canceled_;
  decltype(entity::parent_id_) entity_parent_id_;
  decltype(entity::source_id_) entity_source_id_;
  decltype(task_type::name_) task_type_name_;
  decltype(task_type::for_entity_) task_type_for_entity_;
  decltype(task_type::color_) task_type_color_;

  decltype(task_status::name_) task_status_name_;
  decltype(task_status::color_) task_status_color_;
  decltype(task_status::short_name_) task_status_short_name_;

  std::vector<uuid> assignees_;

  struct comment_t {
    std::string text_;
    chrono::system_zoned_time date_;
    uuid person_id_;
    // to json
    friend void to_json(nlohmann::json& j, const comment_t& p) {
      j["text"]      = p.text_;
      j["date"]      = p.date_;
      j["person_id"] = p.person_id_;
    }
  };

  std::optional<comment_t> last_comment_;
  // to json
  friend void to_json(nlohmann::json& j, const todo_t& p) {
    j["id"]                     = p.uuid_id_;
    j["name"]                   = p.name_;
    j["description"]            = p.description_;
    j["priority"]               = p.priority_;
    j["difficulty"]             = p.difficulty_;
    j["duration"]               = p.duration_;
    j["estimation"]             = p.estimation_;
    j["completion_rate"]        = p.completion_rate_;
    j["retake_count"]           = p.retake_count_;
    j["sort_order"]             = p.sort_order_;
    j["start_date"]             = p.start_date_;
    j["due_date"]               = p.due_date_;
    j["real_start_date"]        = p.real_start_date_;
    j["end_date"]               = p.end_date_;
    j["done_date"]              = p.done_date_;
    j["last_comment_date"]      = p.last_comment_date_;
    j["nb_assets_ready"]        = p.nb_assets_ready_;
    j["data"]                   = p.data_;
    j["shotgun_id"]             = p.shotgun_id_;
    j["last_preview_file_id"]   = p.last_preview_file_id_;
    j["task_type_id"]           = p.task_type_id_;
    j["task_status_id"]         = p.task_status_id_;
    j["assigner_id"]            = p.assigner_id_;
    j["assignees"]              = p.assignees_;
    j["created_at"]             = p.created_at_;
    j["updated_at"]             = p.updated_at_;

    j["project_name"]           = p.project_name_;
    j["project_id"]             = p.project_id_;
    j["project_has_avatar"]     = p.project_has_avatar_;

    j["entity_id"]              = p.entity_uuid_id_;
    j["entity_name"]            = p.entity_name_;
    j["entity_description"]     = p.entity_description_;
    j["entity_data"]            = p.entity_data_;
    j["entity_preview_file_id"] = p.entity_preview_file_id_;
    j["entity_source_id"]       = p.entity_source_id_;
    j["entity_type_name"]       = p.asset_type_name_;
    j["entity_canceled"]        = p.entity_canceled_;

    j["sequence_name"]          = nlohmann::json::value_t::null;
    j["episode_id"]             = "";
    j["episode_name"]           = nlohmann::json::value_t::null;

    j["task_estimation"]        = p.estimation_;
    j["task_duration"]          = p.duration_;
    j["task_start_date"]        = p.start_date_;
    j["task_due_date"]          = p.due_date_;

    j["task_type_name"]         = p.task_type_name_;
    j["task_type_for_entity"]   = p.task_type_for_entity_;
    j["task_status_name"]       = p.task_status_name_;
    j["task_type_color"]        = p.task_type_color_;
    j["task_status_color"]      = p.task_status_color_;
    j["task_status_short_name"] = p.task_status_short_name_;
    if (p.last_comment_)
      j["last_comment"] = p.last_comment_;
    else
      j["last_comment"] = nlohmann::json::value_t::object;
  }
};

}  // namespace doodle
