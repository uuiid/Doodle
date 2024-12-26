//
// Created by TD on 24-12-26.
//

#pragma once
#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/metadata/base.h>

namespace doodle {

struct DOODLE_CORE_API asset_instance : base {
  uuid asset_id_;
  std::string name_;
  std::int32_t number_;
  std::string description_;
  bool active_;
  nlohmann::json data_;
  uuid scene_id_;
  uuid target_asset_id_;
};
}  // namespace doodle