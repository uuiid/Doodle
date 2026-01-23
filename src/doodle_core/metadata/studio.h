//
// Created by TD on 25-3-18.
//

#pragma once

#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/metadata/base.h>
namespace doodle {
struct DOODLE_CORE_API studio {
  DOODLE_BASE_FIELDS();
  std::string name_;
  std::string color_;
  std::string app_key_;
  std::string app_secret_;
  bool archived_;
  // to json
  friend void to_json(nlohmann::json& j, const studio& p) {
    j["id"]       = p.uuid_id_;
    j["name"]     = p.name_;
    j["color"]    = p.color_;
    j["archived"] = p.archived_;
  }
  // from json
  friend void from_json(const nlohmann::json& j, studio& p) {
    if (j.contains("name")) j.at("name").get_to(p.name_);
    if (j.contains("color")) j.at("color").get_to(p.color_);
    if (j.contains("archived")) j.at("archived").get_to(p.archived_);
  }
};

// 外包按照工作室授权
struct DOODLE_CORE_API outsource_studio_authorization {
  DOODLE_BASE_FIELDS();
  uuid studio_id_;  // 外包公司id
  uuid entity_id_;   // 关联实体id, 资产,镜头等
  // to json
  friend void to_json(nlohmann::json& j, const outsource_studio_authorization& p) {
    j["id"]        = p.uuid_id_;
    j["studio_id"] = p.studio_id_;
    j["entity_id"] = p.entity_id_;
  }
  // from json
  friend void from_json(const nlohmann::json& j, outsource_studio_authorization& p) {
    if (j.contains("studio_id")) j.at("studio_id").get_to(p.studio_id_);
    if (j.contains("entity_id")) j.at("entity_id").get_to(p.entity_id_);
  }
};
}  // namespace doodle