//
// Created by TD on 24-12-25.
//

#pragma once

#include <doodle_core/doodle_core_fwd.h>

namespace doodle {

#define DOODLE_BASE_FIELDS() \
  std::int64_t id_;          \
  uuid uuid_id_;             \
  operator bool() const { return id_ != 0 && !uuid_id_.is_nil(); }

}  // namespace doodle