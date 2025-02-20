//
// Created by TD on 24-12-25.
//

#pragma once
#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/metadata/base.h>
namespace doodle {
struct DOODLE_CORE_API department {
  DOODLE_BASE_FIELDS();
  std::string name_;
  std::string color_;
  bool archived_;
  // from json
  template <typename BasicJsonType>
  friend void from_json(const BasicJsonType& j, department& p) {
    j.at("name").get_to(p.name_);
    j.at("color").get_to(p.color_);
    j.at("archived").get_to(p.archived_);
  }
};
}  // namespace doodle