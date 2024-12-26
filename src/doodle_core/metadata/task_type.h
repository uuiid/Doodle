//
// Created by TD on 24-12-26.
//

#pragma once
#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/metadata/base.h>

namespace doodle {

struct task_type : base {
  std::string name_;
  std::string short_name_;
  std::string description_;
  std::string color_;
  std::int32_t priority_;
  std::string for_entity_;
  bool allow_timelog_;
  bool archived_;
  std::int32_t shotgun_id_;
};
}  // namespace doodle