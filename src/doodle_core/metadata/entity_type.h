//
// Created by TD on 24-12-26.
//

#pragma once
#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/metadata/base.h>

namespace doodle {
struct DOODLE_CORE_API task_type_asset_type_link {
  std::int64_t id_;
  uuid asset_type_id_;
  uuid task_type_id_;
};
struct DOODLE_CORE_API asset_type {
  DOODLE_BASE_FIELDS();

  std::string name_;
  std::string short_name_;
  std::string description_;
  std::vector<uuid> task_types_;
  bool archived_;

  // from json
  template <typename BasicJsonType>
  friend void from_json(const BasicJsonType& j, asset_type& p) {
    j.at("name").get_to(p.name_);
    j.at("short_name").get_to(p.short_name_);
    j.at("description").get_to(p.description_);
    if (j.contains("task_types")) j.at("task_types").get_to(p.task_types_);
    j.at("archived").get_to(p.archived_);
  }
  // to json
  template <typename BasicJsonType>
  friend void to_json(BasicJsonType& j, const asset_type& p) {
    j["id"]         = p.uuid_id_;
    j["name"]       = p.name_;
    j["short_name"] = p.short_name_;
    if (p.description_.empty())
      j["description"] = nlohmann::json::value_t::null;
    else
      j["description"] = p.description_;
    j["task_types"] = p.task_types_;
    j["archived"]   = p.archived_;
  }
};
}  // namespace doodle
