//
// Created by TD on 25-8-21.
//

#pragma once
#include <doodle_core/metadata/task.h>
namespace doodle::http {

struct actions_projects_task_types_create_tasks_result : task {
  explicit actions_projects_task_types_create_tasks_result(
      const task& in_task, const task_type& in_type, const task_status& in_status
  )
      : task(in_task),
        task_status_id_(in_status.uuid_id_),
        task_status_name_(in_status.name_),
        task_status_short_name_(in_status.short_name_),
        task_status_color_(in_status.color_),
        task_type_id_(in_type.uuid_id_),
        task_type_name_(in_type.name_),
        task_type_color_(in_type.color_),
        task_type_priority_(in_type.priority_) {}

  uuid task_status_id_;
  std::string task_status_name_;
  std::string task_status_short_name_;
  std::string task_status_color_;
  uuid task_type_id_;
  std::string task_type_name_;
  std::string task_type_color_;
  std::int32_t task_type_priority_;
  // to json
  friend void to_json(nlohmann::json& j, const actions_projects_task_types_create_tasks_result& p) {
    to_json(j, static_cast<const task&>(p));
    j["task_status_id"]         = p.task_status_id_;
    j["task_status_name"]       = p.task_status_name_;
    j["task_status_short_name"] = p.task_status_short_name_;
    j["task_status_color"]      = p.task_status_color_;
    j["task_type_id"]           = p.task_type_id_;
    j["task_type_name"]         = p.task_type_name_;
    j["task_type_color"]        = p.task_type_color_;
    j["task_type_priority"]     = p.task_type_priority_;
  }
};

}  // namespace doodle::http