//
// Created by TD on 25-5-8.
//

#pragma once
#include <doodle_core/metadata/base.h>
namespace doodle {
struct Label {
  DOODLE_BASE_FIELDS();
  std::string label_;
  // to json
  friend void to_json(nlohmann::json& j, const Label& p) {
    j["id"]    = p.uuid_id_;
    j["label"] = p.label_;
  }
  friend void from_json(const nlohmann::json& j, Label& p) { j.at("label").get_to(p.label_); }
};
}  // namespace doodle