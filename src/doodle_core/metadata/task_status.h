//
// Created by TD on 24-12-26.
//

#pragma once
#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/metadata/base.h>

namespace doodle {
struct task;
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

  void check_retake_capping(const task& in_task);
  static const uuid& get_completed();
  // nearly completed
  static const uuid& get_nearly_completed();
  static const uuid& get_to_do();

  // form json
  friend void from_json(const nlohmann::json& in_json, task_status& out_obj) {
    if (in_json.contains("name")) in_json.at("name").get_to(out_obj.name_);
    if (in_json.contains("archived")) in_json.at("archived").get_to(out_obj.archived_);
    if (in_json.contains("short_name")) in_json.at("short_name").get_to(out_obj.short_name_);
    if (in_json.contains("description")) in_json.at("description").get_to(out_obj.description_);
    if (in_json.contains("color")) in_json.at("color").get_to(out_obj.color_);
    if (in_json.contains("priority")) in_json.at("priority").get_to(out_obj.priority_);
    if (in_json.contains("is_done")) in_json.at("is_done").get_to(out_obj.is_done_);
    if (in_json.contains("is_artist_allowed")) in_json.at("is_artist_allowed").get_to(out_obj.is_artist_allowed_);
    if (in_json.contains("is_client_allowed")) in_json.at("is_client_allowed").get_to(out_obj.is_client_allowed_);
    if (in_json.contains("is_retake")) in_json.at("is_retake").get_to(out_obj.is_retake_);
    if (in_json.contains("is_feedback_request")) in_json.at("is_feedback_request").get_to(out_obj.is_feedback_request_);
    if (in_json.contains("is_default")) in_json.at("is_default").get_to(out_obj.is_default_);
    if (in_json.contains("shotgun_id")) in_json.at("shotgun_id").get_to(out_obj.shotgun_id_);
    if (in_json.contains("for_concept")) in_json.at("for_concept").get_to(out_obj.for_concept_);
  }
  // to json
  friend void to_json(nlohmann::json& in_json, const task_status& out_obj) {
    in_json["id"]                  = out_obj.uuid_id_;
    in_json["name"]                = out_obj.name_;
    in_json["archived"]            = out_obj.archived_;
    in_json["short_name"]          = out_obj.short_name_;
    in_json["description"]         = out_obj.description_;
    in_json["color"]               = out_obj.color_;
    in_json["priority"]            = out_obj.priority_;
    in_json["is_done"]             = out_obj.is_done_;
    in_json["is_artist_allowed"]   = out_obj.is_artist_allowed_;
    in_json["is_client_allowed"]   = out_obj.is_client_allowed_;
    in_json["is_retake"]           = out_obj.is_retake_;
    in_json["is_feedback_request"] = out_obj.is_feedback_request_;
    in_json["is_default"]          = out_obj.is_default_;
    in_json["shotgun_id"]          = out_obj.shotgun_id_;
    in_json["for_concept"]         = out_obj.for_concept_;
  }
};
}  // namespace doodle