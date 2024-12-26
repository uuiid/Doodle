//
// Created by TD on 24-12-26.
//

#pragma once
#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/metadata/base.h>

namespace doodle {
struct DOODLE_CORE_API task_type_asset_type_link {
  std::int64_t id_;
  uuid asset_type_id_;
  uuid task_type_id_;
};
struct DOODLE_CORE_API asset_type {
  DOODLE_BASE_FIELDS();

  std::string name_;
  std::string short_name_;
  std::string description_;
  std::vector<uuid> task_types_;
  bool archived_;
};
}  // namespace doodle
