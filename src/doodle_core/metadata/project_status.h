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

  constexpr static uuid get_open_id() {
    return uuid{{0x75, 0x5c, 0x9e, 0xdd, 0x94, 0x81, 0x41, 0x45, 0xab, 0x43, 0x21, 0x49, 0x1b, 0xdf, 0x27, 0x39}};
  }
  constexpr static uuid get_closed_id() {
    return uuid{{0x51, 0x59, 0xf2, 0x10, 0x7e, 0xc8, 0x40, 0xe3, 0xb8, 0xc9, 0x2a, 0x06, 0xd0, 0xb4, 0xb1, 0x16}};
  }
  // 获取所以的常量项目状态
  constexpr static std::array<project_status, 2> get_all_constant() {
    return {
        project_status{.uuid_id_ = get_open_id(), .name_ = "Open", .color_ = "#000000"},
        project_status{.uuid_id_ = get_closed_id(), .name_ = "Closed", .color_ = "#000000"}
    };
  }

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