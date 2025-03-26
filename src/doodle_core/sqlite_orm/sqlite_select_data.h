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
};
}  // namespace doodle
