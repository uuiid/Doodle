#pragma once

#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/metadata/base.h>

#include <string>

namespace doodle::seedance2 {
struct DOODLE_CORE_API assets_entity_item {
  DOODLE_BASE_FIELDS();
  uuid parent_id_;
  std::string file_extension_;

  // to json
  friend void to_json(nlohmann::json& j, const assets_entity_item& p) {
    j["id"]             = p.uuid_id_;
    j["entity_id"]      = p.parent_id_;
    j["asset_id"]       = p.uuid_id_;
    j["file_extension"] = p.file_extension_;
  }
};
}  // namespace doodle::seedance2