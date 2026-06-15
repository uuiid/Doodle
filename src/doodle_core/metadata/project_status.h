//
// Created by TD on 24-12-26.
//

#pragma once
#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/metadata/base.h>

namespace doodle {
/**
 * 描述项目的状态（主要是打开或关闭）。
 */
struct DOODLE_CORE_API project_status {
  DOODLE_BASE_FIELDS();
  std::string name_;
  std::string color_;

  constexpr static uuid get_open_id();
  constexpr static uuid get_closed_id();
  // 获取所以的常量项目状态
  constexpr static std::array<project_status, 2> get_all_constant();

  // from json
  friend void from_json(const nlohmann::json& j, project_status& p) {
    j.at("name").get_to(p.name_);
    j.at("color").get_to(p.color_);
  }
  // to json
  friend void to_json(nlohmann::json& j, const project_status& p) {
    j["id"]    = p.uuid_id_;
    j["name"]  = p.name_;
    j["color"] = p.color_;
  }
};

}  // namespace doodle