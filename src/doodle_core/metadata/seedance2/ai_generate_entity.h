#pragma once

#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/metadata/base.h>

#include <string>

namespace doodle::seedance2 {
/// 这个是 ai 生成中的实体, 包含有一系列 task 任务, 用来生成视频或者图片
struct DOODLE_CORE_API ai_generate_entity {
  DOODLE_BASE_FIELDS();
  std::string name_;          // 必填
  uuid ai_episode_id_;        // 必填
  std::string description_;   // 选填

  uuid shot_uuid_id_;     // 选填 内部使用的UUID，对应镜头中的uuid_id_
  uuid project_uuid_id_;  // 必填 内部使用的UUID，对应项目中的uuid_id_
  uuid preview_file_;     // 选填 对应 ai_preview_file
  // to json
  friend void to_json(nlohmann::json& j, const ai_generate_entity& p) {
    j["id"]              = p.uuid_id_;
    j["name"]            = p.name_;
    j["ai_episode_id"]   = p.ai_episode_id_;
    j["description"]     = p.description_;
    j["shot_uuid_id"]    = p.shot_uuid_id_;
    j["project_uuid_id"] = p.project_uuid_id_;
    j["preview_file"]    = p.preview_file_;
  }
  // from json
  friend void from_json(const nlohmann::json& j, ai_generate_entity& p) {
    j.at("name").get_to(p.name_);
    j.at("ai_episode_id").get_to(p.ai_episode_id_);
    j.at("project_uuid_id").get_to(p.project_uuid_id_);
    if (j.contains("description")) j.at("description").get_to(p.description_);
    if (j.contains("shot_uuid_id")) j.at("shot_uuid_id").get_to(p.shot_uuid_id_);
    if (j.contains("preview_file")) j.at("preview_file").get_to(p.preview_file_);
  }
};

/// 实例参考, 链接到缩略图
struct DOODLE_CORE_API ai_entity_reference_preview {
  DOODLE_BASE_FIELDS();
  uuid ai_generate_entity_id_;  // 必填
  uuid preview_file_;           // 必填 对应 ai_preview_file
  // to json
  friend void to_json(nlohmann::json& j, const ai_entity_reference_preview& p) {
    j["id"]                    = p.uuid_id_;
    j["ai_generate_entity_id"] = p.ai_generate_entity_id_;
    j["preview_file"]          = p.preview_file_;
  }
  // from json
  friend void from_json(const nlohmann::json& j, ai_entity_reference_preview& p) {
    j.at("ai_generate_entity_id").get_to(p.ai_generate_entity_id_);
    j.at("preview_file").get_to(p.preview_file_);
  }
};

}  // namespace doodle::seedance2