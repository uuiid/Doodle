//
// Created by TD on 25-5-8.
//

#pragma once
#include <doodle_core/metadata/base.h>
namespace doodle {

struct label_assets_link {
  std::int32_t id_{};
  uuid label_uuid_id_{};
  uuid assets_uuid_id_{};
};

struct label {
  DOODLE_BASE_FIELDS();
  std::string label_;
  // to json
  friend void to_json(nlohmann::json& j, const label& p) {
    j["id"]    = p.uuid_id_;
    j["name"] = p.label_;
  }
  friend void from_json(const nlohmann::json& j, label& p) { j.at("name").get_to(p.label_); }
};
}  // namespace doodle