//
// Created by TD on 24-12-26.
//

#pragma once
#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/metadata/base.h>

namespace doodle {

struct DOODLE_CORE_API task_status {
  DOODLE_BASE_FIELDS();

  std::string name_;
  bool archived_;
  std::string short_name_;
  std::string description_;
  std::string color_;
  std::int32_t priority_;
  bool is_done_;
  bool is_artist_allowed_;
  bool is_client_allowed_;
  bool is_retake_;
  bool is_feedback_request_;
  bool is_default_;
  std::optional<std::int32_t> shotgun_id_;
  bool for_concept_;

  // form json
  friend void from_json(const nlohmann::json& in_json, task_status& out_obj) {
    in_json.at("name").get_to(out_obj.name_);
    in_json.at("archived").get_to(out_obj.archived_);
    in_json.at("short_name").get_to(out_obj.short_name_);
    in_json.at("description").get_to(out_obj.description_);
    in_json.at("color").get_to(out_obj.color_);
    in_json.at("priority").get_to(out_obj.priority_);
    in_json.at("is_done").get_to(out_obj.is_done_);
    in_json.at("is_artist_allowed").get_to(out_obj.is_artist_allowed_);
    in_json.at("is_client_allowed").get_to(out_obj.is_client_allowed_);
    in_json.at("is_retake").get_to(out_obj.is_retake_);
    in_json.at("is_feedback_request").get_to(out_obj.is_feedback_request_);
    in_json.at("is_default").get_to(out_obj.is_default_);
    in_json.at("shotgun_id").get_to(out_obj.shotgun_id_);
    in_json.at("for_concept").get_to(out_obj.for_concept_);
  }
};
}  // namespace doodle