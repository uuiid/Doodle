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
  std::string name_{};
  std::string description_{};
  std::int32_t priority_;
  std::int32_t difficulty_;
  std::float_t duration_;
  std::float_t estimation_;
  std::int32_t completion_rate_;
  std::int32_t retake_count_;
  std::int32_t sort_order_;
  chrono::system_zoned_time start_date_;
  chrono::system_zoned_time due_date_;
  chrono::system_zoned_time real_start_date_;
  chrono::system_zoned_time end_date_;
  chrono::system_zoned_time done_date_;
  chrono::system_zoned_time last_comment_date_;
  std::int32_t nb_assets_ready_;
  nlohmann::json data_;
  std::int32_t shotgun_id_;
  uuid last_preview_file_id_;
  std::int32_t nb_drawings_;

  // 外键
  uuid project_id_;
  uuid task_type_id_;
  uuid task_status_id_;
  uuid entity_id_;
  uuid assigner_id_;
  // 一对多(任务分配的人员)
  std::vector<uuid> assignees_{};
};
}  // namespace doodle