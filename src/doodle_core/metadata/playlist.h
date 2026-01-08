//
// Created by TD on 25-8-25.
//

#pragma once
#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/metadata/base.h>

#include <chrono>
namespace doodle {
struct DOODLE_CORE_API playlist {
  DOODLE_BASE_FIELDS();
  std::string name_;
  nlohmann::json shots_;

  uuid project_id_;
  uuid episodes_id_;
  uuid task_type_id_;
  bool for_client_;
  std::string for_entity_;
  bool is_for_all_;

  chrono::system_zoned_time created_at_{chrono::current_zone(), chrono::system_clock::now()};
  chrono::system_zoned_time updated_at_{chrono::current_zone(), chrono::system_clock::now()};
  // to json
  friend void to_json(nlohmann::json& j, const playlist& p) {
    j["id"]           = p.uuid_id_;
    j["name"]         = p.name_;
    j["project_id"]   = p.project_id_;
    j["episodes_id"]  = p.episodes_id_;
    j["task_type_id"] = p.task_type_id_;
    j["for_client"]   = p.for_client_;
    j["is_for_all"]   = p.is_for_all_;
    j["for_entity"]   = p.for_entity_;
    j["created_at"]   = p.created_at_;
    j["updated_at"]   = p.updated_at_;
  }
  // from json
  friend void from_json(const nlohmann::json& j, playlist& p) {
    j.at("name").get_to(p.name_);
    if (j.contains("project_id")) j.at("project_id").get_to(p.project_id_);
    if (j.contains("task_type_id")) j.at("task_type_id").get_to(p.task_type_id_);
    j.at("for_client").get_to(p.for_client_);
    j.at("for_entity").get_to(p.for_entity_);
    if (j.contains("episodes_id")) j.at("episodes_id").get_to(p.episodes_id_);
    if (j.contains("is_for_all") && j.at("is_for_all").is_boolean()) j.at("is_for_all").get_to(p.is_for_all_);
  }
};

struct DOODLE_CORE_API playlist_shot {
  DOODLE_BASE_FIELDS();

  uuid playlist_id_;
  uuid entity_id_;
  uuid preview_id_;
  std::int32_t order_index_;

  // from json
  friend void from_json(const nlohmann::json& j, playlist_shot& p) {
    if (j.contains("entity_id") && j.at("entity_id").is_string()) j["entity_id"].get_to(p.entity_id_);
    if (j.contains("sequence_id") && j.at("sequence_id").is_string()) j["sequence_id"].get_to(p.entity_id_);

    if (j.contains("preview_file_id") && j.at("preview_file_id").is_string())
      j["preview_file_id"].get_to(p.preview_id_);
    if (j.contains("order_index") && j.at("order_index").is_number_integer()) j["order_index"].get_to(p.order_index_);
  }
  // to json
  friend void to_json(nlohmann::json& j, const playlist_shot& p) {
    j["id"]              = p.uuid_id_;
    j["shot_id"]         = p.uuid_id_;
    j["entity_id"]       = p.entity_id_;
    j["preview_file_id"] = p.preview_id_;
    j["order_index"]     = p.order_index_;
  }
};
}  // namespace doodle