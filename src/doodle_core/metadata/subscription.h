//
// Created by TD on 25-3-26.
//

#pragma once

#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/metadata/base.h>

namespace doodle {
struct DOODLE_CORE_API subscription {
  DOODLE_BASE_FIELDS();
  uuid person_id_;
  uuid task_id_;
  uuid entity_id_;
  uuid task_type_id_;
};
}  // namespace doodle