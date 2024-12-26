//
// Created by TD on 24-12-26.
//

#pragma once
#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/metadata/base.h>

namespace doodle {
enum class status_automation_change_type { status, ready_for };
struct status_automation {
  DOODLE_BASE_FIELDS();
  std::string entity_type_;
  uuid in_task_type_id_;
  uuid in_task_status_id_;
  status_automation_change_type out_field_type_;
  uuid out_task_type_id_;
  uuid out_task_status_id_;
  bool import_last_revision_;
  bool archived_;
};
}  // namespace doodle