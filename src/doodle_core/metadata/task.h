//
// Created by TD on 25-3-11.
//

#pragma once

#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/metadata/base.h>
namespace doodle {

struct assignees_table {
  std::int64_t id_;
  uuid task_id_;
  uuid person_id_;
};

struct DOODLE_CORE_API task {
  DOODLE_BASE_FIELDS();
  std::string name_;
  std::string description_;
  std::int32_t priority_;
  std::int32_t difficulty_{3};
  std::float_t duration_;
  std::float_t estimation_;
  std::int32_t completion_rate_;
  std::int32_t retake_count_;
  std::int32_t sort_order_;
  std::optional<chrono::system_zoned_time> start_date_;
  std::optional<chrono::system_zoned_time> due_date_;
  std::optional<chrono::system_zoned_time> real_start_date_;  // 实际开始时间
  std::optional<chrono::system_zoned_time> end_date_;
  std::optional<chrono::system_zoned_time> done_date_;
  std::optional<chrono::system_zoned_time> last_comment_date_;
  std::int32_t nb_assets_ready_;
  nlohmann::json data_;
  std::int32_t shotgun_id_;
  uuid last_preview_file_id_;
  std::int32_t nb_drawings_;

  chrono::system_zoned_time created_at_{chrono::system_clock::now()};
  chrono::system_zoned_time updated_at_{chrono::system_clock::now()};

  // 外键
  uuid project_id_;
  uuid task_type_id_;
  uuid task_status_id_;
  uuid entity_id_;
  uuid assigner_id_;
  // 一对多(任务分配的人员)
  std::vector<uuid> assignees_{};

  // to json
  friend void to_json(nlohmann::json& j, const task& p) {
    j["id"]                   = p.uuid_id_;
    j["name"]                 = p.name_;
    j["description"]          = p.description_;
    j["priority"]             = p.priority_;
    j["difficulty"]           = p.difficulty_;
    j["duration"]             = p.duration_;
    j["estimation"]           = p.estimation_;
    j["completion_rate"]      = p.completion_rate_;
    j["retake_count"]         = p.retake_count_;
    j["sort_order"]           = p.sort_order_;
    j["start_date"]           = p.start_date_;
    j["due_date"]             = p.due_date_;
    j["real_start_date"]      = p.real_start_date_;
    j["end_date"]             = p.end_date_;
    j["done_date"]            = p.done_date_;
    j["last_comment_date"]    = p.last_comment_date_;
    j["nb_assets_ready"]      = p.nb_assets_ready_;
    j["data"]                 = p.data_;
    j["shotgun_id"]           = p.shotgun_id_;
    j["last_preview_file_id"] = p.last_preview_file_id_;
    j["nb_drawings"]          = p.nb_drawings_;

    j["project_id"]           = p.project_id_;
    j["task_type_id"]         = p.task_type_id_;
    j["task_status_id"]       = p.task_status_id_;
    j["entity_id"]            = p.entity_id_;
    j["assigner_id"]          = p.assigner_id_;

    j["assignees"]            = p.assigner_id_;
  }
  // from json(此处针对put方法, 不针对post方法)
  friend void from_json(const nlohmann::json& j, task& p) {
    if (j.contains("name")) j.at("name").get_to(p.name_);
    if (j.contains("description")) j.at("description").get_to(p.description_);
    if (j.contains("priority")) j.at("priority").get_to(p.priority_);
    if (j.contains("difficulty")) j.at("difficulty").get_to(p.difficulty_);
    if (j.contains("duration")) j.at("duration").get_to(p.duration_);
    if (j.contains("estimation")) j.at("estimation").get_to(p.estimation_);
    if (j.contains("completion_rate")) j.at("completion_rate").get_to(p.completion_rate_);
    if (j.contains("sort_order")) j.at("sort_order").get_to(p.sort_order_);
    if (j.contains("start_date")) j.at("start_date").get_to(p.start_date_);
    if (j.contains("due_date")) j.at("due_date").get_to(p.due_date_);
    if (j.contains("real_start_date")) j.at("real_start_date").get_to(p.real_start_date_);
    if (j.contains("end_date")) j.at("end_date").get_to(p.end_date_);
    if (j.contains("done_date")) j.at("done_date").get_to(p.done_date_);
    if (j.contains("last_comment_date")) j.at("last_comment_date").get_to(p.last_comment_date_);
    if (j.contains("nb_assets_ready")) j.at("nb_assets_ready").get_to(p.nb_assets_ready_);
    if (j.contains("data")) j.at("data").get_to(p.data_);
    p.updated_at_ = chrono::system_clock::now();
  }
};
}  // namespace doodle