#pragma once

#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/metadata/base.h>

#include <string>

namespace doodle::seedance2 {
///
struct DOODLE_CORE_API ai_preview_file {
  DOODLE_BASE_FIELDS();
  std::string extension_;  // 必填
  // to json
  friend void to_json(nlohmann::json& j, const ai_preview_file& p) {
    j["id"]        = p.uuid_id_;
    j["extension"] = p.extension_;
  }
  // from json
  friend void from_json(const nlohmann::json& j, ai_preview_file& p) {
    j.at("extension").get_to(p.extension_);
  }
};
}  // namespace doodle::seedance2