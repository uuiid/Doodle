//
// Created by TD on 24-12-26.
//

#pragma once
#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/metadata/base.h>

namespace doodle {

struct task_type {
  DOODLE_BASE_FIELDS();

  std::string name_;
  std::string short_name_;
  std::string description_;
  std::string color_;
  std::int32_t priority_;
  std::string for_entity_;
  bool allow_timelog_;
  bool archived_;
  std::optional<std::int32_t> shotgun_id_;
  uuid department_id_;

  // from json
  template <typename BasicJsonType>
  friend void from_json(const BasicJsonType& j, task_type& p) {
    j.at("name").get_to(p.name_);
    j.at("short_name").get_to(p.short_name_);
    j.at("description").get_to(p.description_);
    j.at("color").get_to(p.color_);
    j.at("priority").get_to(p.priority_);
    j.at("for_entity").get_to(p.for_entity_);
    j.at("allow_timelog").get_to(p.allow_timelog_);
    j.at("archived").get_to(p.archived_);
    j.at("shotgun_id").get_to(p.shotgun_id_);
    j.at("department_id").get_to(p.department_id_);
  }
  // to json
  template <typename BasicJsonType>
  friend void to_json(BasicJsonType& j, const task_type& p) {
    j["id"]            = p.uuid_id_;
    j["name"]          = p.name_;
    j["short_name"]    = p.short_name_;
    j["description"]   = p.description_;
    j["color"]         = p.color_;
    j["priority"]      = p.priority_;
    j["for_entity"]    = p.for_entity_;
    j["allow_timelog"] = p.allow_timelog_;
    j["archived"]      = p.archived_;
    j["shotgun_id"]    = p.shotgun_id_;
    j["department_id"] = p.department_id_;
  }
};
}  // namespace doodle