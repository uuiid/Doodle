//
// Created by TD on 24-11-11.
//

#pragma once
#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/metadata/assets.h>

namespace doodle::metadata::kitsu {

struct assets_type_t {
  std::int32_t id_{};
  uuid uuid_id_{};

  std::string name_{};
  details::assets_type_enum type_{details::assets_type_enum::other};
};

}  // namespace doodle::metadata::kitsu