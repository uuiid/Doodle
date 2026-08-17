#pragma once

#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/metadata/base.h>

#include <string>

namespace doodle::seedance2 {
///
struct DOODLE_CORE_API ai_generate_entity {
  DOODLE_BASE_FIELDS();
  std::string name_;                    // 必填
  uuid ai_generate_classification_id_;  // 必填

  uuid shot_uuid_id_;     // 选填 内部使用的UUID，对应镜头中的uuid_id_
  uuid project_uuid_id_;  // 必填 内部使用的UUID，对应项目中的uuid_id_
  uuid preview_file_;     // 选填 对应 ai_preview_file
  // to json
  friend void to_json(nlohmann::json& j, const ai_generate_entity& p) {
    j["id"]                            = p.uuid_id_;
    j["name"]                          = p.name_;
    j["ai_generate_classification_id"] = p.ai_generate_classification_id_;
    j["shot_uuid_id"]                  = p.shot_uuid_id_;
    j["project_uuid_id"]               = p.project_uuid_id_;
    j["preview_file"]                  = p.preview_file_;
  }
  // from json
  friend void from_json(const nlohmann::json& j, ai_generate_entity& p) {
    j.at("name").get_to(p.name_);
    j.at("ai_generate_classification_id").get_to(p.ai_generate_classification_id_);
    j.at("project_uuid_id").get_to(p.project_uuid_id_);
    if (j.contains("shot_uuid_id")) j.at("shot_uuid_id").get_to(p.shot_uuid_id_);
    if (j.contains("preview_file")) j.at("preview_file").get_to(p.preview_file_);
  }
};

}  // namespace doodle::seedance2