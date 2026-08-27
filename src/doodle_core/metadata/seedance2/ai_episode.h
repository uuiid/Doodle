#pragma once

#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/metadata/base.h>

#include <string>
#include <tuple>
#include <utility>

namespace doodle::seedance2 {
/// 这个是 ai 生成中的剧集，比如 "sc002", "sc003" 等等
struct DOODLE_CORE_API ai_episode {
  DOODLE_BASE_FIELDS();
  std::string name_;
  std::string description_;
  uuid subproject_id_;

  uuid entity_id_;  // 对应的 entity.h

  constexpr static auto put_property_list() {
    return std::tuple{
        std::pair{"name", &ai_episode::name_},  //
        std::pair{"description", &ai_episode::description_}
    };
  }

  chrono::system_zoned_time created_at_{chrono::current_zone(), chrono::system_clock::now()};
  // to json
  friend void to_json(nlohmann::json& j, const ai_episode& p) {
    j["id"]            = p.uuid_id_;
    j["name"]          = p.name_;
    j["description"]   = p.description_;
    j["created_at"]    = p.created_at_;
    j["subproject_id"] = p.subproject_id_;
    j["entity_id"]     = p.entity_id_;
  }
  // from json
  friend void from_json(const nlohmann::json& j, ai_episode& p) {
    j.at("name").get_to(p.name_);
    j.at("subproject_id").get_to(p.subproject_id_);
    if (j.contains("description")) j.at("description").get_to(p.description_);
    if (j.contains("entity_id")) j.at("entity_id").get_to(p.entity_id_);
  }
};

// 限制 ai_episode 中, 可以使用的模型和分辨率
struct DOODLE_CORE_API ai_episode_model_resolution_limit {
  DOODLE_BASE_FIELDS();
  uuid ai_episode_id_;      // < 以下三项有唯一约束
  std::string model_name_;  //
  std::string resolution_;  //
  // to json
  friend void to_json(nlohmann::json& j, const ai_episode_model_resolution_limit& p) {
    j["id"]            = p.uuid_id_;
    j["ai_episode_id"] = p.ai_episode_id_;
    j["model_name"]    = p.model_name_;
    j["resolution"]    = p.resolution_;
  }
  // from json
  friend void from_json(const nlohmann::json& j, ai_episode_model_resolution_limit& p) {
    j.at("model_name").get_to(p.model_name_);
    j.at("resolution").get_to(p.resolution_);
  }
};

}  // namespace doodle::seedance2
