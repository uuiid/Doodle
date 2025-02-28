//
// Created by TD on 24-9-28.
//

#pragma once
#include <doodle_core/doodle_core_fwd.h>
namespace doodle::metadata::kitsu {

struct task_type_t {
  std::int32_t id_{};
  uuid uuid_id_{};

  std::string name_{};
  bool use_chick_files{false};
};

}  // namespace doodle::metadata::kitsu