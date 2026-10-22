#pragma once
#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/metadata/base.h>
#include <doodle_core/metadata/seedance2/task_backend.h>

#include <chrono>
#include <magic_enum/magic_enum.hpp>
#include <string>
#include <string_view>

namespace doodle::seedance2 {

enum class DOODLE_CORE_API task_status {
  preparing,
  queued,
  running,
  cancelled,
  succeeded,
  failed,
  violation,
  expired,
};

enum class DOODLE_CORE_API task_type {
  picture,
  video,
  inference,
};
DOODLE_JSON_SERIALIZE_ENUM_STRICT(
    task_type,
    {{task_type::picture, "picture"}, {task_type::video, "video"}, {task_type::inference, "inference"}}
);
DOODLE_JSON_SERIALIZE_ENUM_STRICT(
    task_status,

    {{task_status::preparing, "preparing"},
     {task_status::queued, "queued"},
     {task_status::running, "running"},
     {task_status::cancelled, "cancelled"},
     {task_status::succeeded, "succeeded"},
     {task_status::failed, "failed"},
     {task_status::violation, "violation"},
     {task_status::expired, "expired"}}
);

struct DOODLE_CORE_API task {
  DOODLE_BASE_FIELDS();
  uuid user_id_;
  task_status status_{task_status::preparing};
  task_type type_{task_type::video};
  task_backend backend_{task_backend::seedance2};
  nlohmann::json data_request_;
  std::string text_prompt_;
  uuid preview_file_;
  std::string file_extension_;
  nlohmann::json data_response_;
  uuid ai_studio_id_;

  std::string task_id_;
  chrono::system_zoned_time created_at_{chrono::current_zone(), chrono::system_clock::now()};
  chrono::system_zoned_time ended_at_{chrono::current_zone(), chrono::system_clock::now()};

  uuid project_uuid_id_;
  std::int64_t completion_tokens_{doodle_config::g_max_task_completion_tokens};
  uuid ai_generate_entity_id_;
  uuid subproject_id_;

  uuid ai_light_session_id_;

  std::int64_t retry_count_{0};
  bool archived_;
  friend void to_json(nlohmann::json& j, const task& p) {
    j["id"]                    = p.uuid_id_;
    j["user_id"]               = p.user_id_;
    j["status"]                = p.status_;
    j["type"]                  = p.type_;
    j["backend"]               = p.backend_;
    j["text_prompt"]           = p.text_prompt_;
    j["preview_file"]          = p.preview_file_;
    j["data_request"]          = p.data_request_;
    j["file_extension"]        = p.file_extension_;
    j["data_response"]         = p.data_response_;
    j["ai_studio_id"]          = p.ai_studio_id_;

    j["created_at"]            = p.created_at_;
    j["ended_at"]              = p.ended_at_;
    j["archived"]              = p.archived_;
    j["project_uuid_id"]       = p.project_uuid_id_;
    j["completion_tokens"]     = p.completion_tokens_;
    j["ai_generate_entity_id"] = p.ai_generate_entity_id_;
    j["subproject_id"]         = p.subproject_id_;
    j["ai_light_session_id"]   = p.ai_light_session_id_;
    j["retry_count"]           = p.retry_count_;
  }
  friend void from_json(const nlohmann::json& j, task& p) {
    j.at("data_request").get_to(p.data_request_);
    j.at("ai_studio_id").get_to(p.ai_studio_id_);
    j.at("project_uuid_id").get_to(p.project_uuid_id_);
    j.at("type").get_to(p.type_);
    j.at("backend").get_to(p.backend_);
    j.at("ai_generate_entity_id").get_to(p.ai_generate_entity_id_);

    if (j.contains("archived")) j.at("archived").get_to(p.archived_);
    if (j.contains("ai_light_session_id")) j.at("ai_light_session_id").get_to(p.ai_light_session_id_);
  }
};

struct DOODLE_CORE_API person_token {
  DOODLE_BASE_FIELDS();
  chrono::year_month_day date_;
  uuid person_id_;
  std::int64_t token_consumed_{0};
  std::int64_t task_count_{0};
  std::int64_t project_count_{0};
  std::int64_t video_count_{0};
  std::int64_t picture_count_{0};
  std::int64_t inference_count_{0};

  friend void to_json(nlohmann::json& j, const person_token& p) {
    j["id"]              = p.uuid_id_;
    j["date"]            = p.date_;
    j["person_id"]       = p.person_id_;
    j["token_amount"]    = p.token_consumed_;
    j["task_count"]      = p.task_count_;
    j["project_count"]   = p.project_count_;
    j["video_count"]     = p.video_count_;
    j["picture_count"]   = p.picture_count_;
    j["inference_count"] = p.inference_count_;
  }
};

struct DOODLE_CORE_API task_similarity {
  DOODLE_BASE_FIELDS();
  uuid task_id_;
  uuid similar_task_id_;
  double similarity_{0.0};

  friend void to_json(nlohmann::json& j, const task_similarity& p) {
    j["id"]              = p.uuid_id_;
    j["task_id"]         = p.task_id_;
    j["similar_task_id"] = p.similar_task_id_;
    j["similarity"]      = p.similarity_;
  }
};

}  // namespace doodle::seedance2
