#pragma once

#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/metadata/base.h>

#include <string>

namespace doodle::seedance2 {

/// AI生成实体类别, 通过 ai_generate_entity_id 与 ai_generate_entity 关联
struct DOODLE_CORE_API ai_category {
  DOODLE_BASE_FIELDS();
  std::string name_;         ///< 类别名称 必填
  std::string description_;  ///< 类别描述 选填
  uuid preview_file_;        ///< 预览图 选填

  // to json
  friend void to_json(nlohmann::json& j, const ai_category& p) {
    j["id"]           = p.uuid_id_;
    j["name"]         = p.name_;
    j["description"]  = p.description_;
    j["preview_file"] = p.preview_file_;
  }
  // from json
  friend void from_json(const nlohmann::json& j, ai_category& p) {
    j.at("name").get_to(p.name_);
    if (j.contains("description")) j.at("description").get_to(p.description_);
    if (j.contains("preview_file")) j.at("preview_file").get_to(p.preview_file_);
  }
};

}  // namespace doodle::seedance2
