#pragma once
#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/metadata/base.h>

#include <chrono>
#include <string>

namespace doodle::seedance2 {
/**
queued：排队中。
running：任务运行中。
cancelled：取消任务，取消状态24h自动删除（只支持排队中状态的任务被取消）。
succeeded： 任务成功。（如发送失败，即5秒内没有接收到成功发送的信息，回调三次）
failed：任务失败。（如发送失败，即5秒内没有接收到成功发送的信息，回调三次）
expired：任务超时，即任务处于运行中或排队中状态超过过期时间。可通过 execution_expires_after 字段设置过期时间。
*/
enum class DOODLE_CORE_API task_status {
  queued,
  running,
  cancelled,
  succeeded,
  failed,
  expired,
};

// 任务类别 图片,视频等
enum class DOODLE_CORE_API task_type {
  picture,
  video,
};
NLOHMANN_JSON_SERIALIZE_ENUM(task_type, {{task_type::picture, "picture"}, {task_type::video, "video"}});
NLOHMANN_JSON_SERIALIZE_ENUM(
    task_status, {{task_status::queued, "queued"},
                  {task_status::running, "running"},
                  {task_status::cancelled, "cancelled"},
                  {task_status::succeeded, "succeeded"},
                  {task_status::failed, "failed"},
                  {task_status::expired, "expired"}}
);
struct DOODLE_CORE_API task {
  DOODLE_BASE_FIELDS();
  uuid user_id_;
  task_status status_;
  task_type type_{task_type::video};
  nlohmann::json data_request_;
  std::string file_extension_;
  nlohmann::json data_response_;
  uuid ai_studio_id_;

  std::string task_id_;  // 任务ID，外部唯一标识
  chrono::system_zoned_time created_at_{chrono::current_zone(), chrono::system_clock::now()};
  chrono::system_zoned_time ended_at_{chrono::current_zone(), chrono::system_clock::now()};

  uuid shot_uuid_id_;                        // 内部使用的UUID，对应镜头中的uuid_id_
  uuid project_uuid_id_;                     // 内部使用的UUID，对应项目中的uuid_id_
  std::int64_t completion_tokens_{20'0000};  // 任务完成所需的token数量，默认为20万，具体数值可根据实际情况调整

  // 归档
  bool archived_;
  // to json
  friend void to_json(nlohmann::json& j, const task& p) {
    j["id"]                = p.uuid_id_;
    j["user_id"]           = p.user_id_;
    j["status"]            = p.status_;
    j["type"]              = p.type_;
    j["data_request"]      = p.data_request_;
    j["file_extension"]    = p.file_extension_;
    j["data_response"]     = p.data_response_;
    j["ai_studio_id"]      = p.ai_studio_id_;

    j["created_at"]        = p.created_at_;
    j["ended_at"]          = p.ended_at_;
    j["archived"]          = p.archived_;
    j["shot_uuid_id"]      = p.shot_uuid_id_;
    j["project_uuid_id"]   = p.project_uuid_id_;
    j["completion_tokens"] = p.completion_tokens_;
  }
  friend void from_json(const nlohmann::json& j, task& p) {
    j.at("data_request").get_to(p.data_request_);
    j.at("ai_studio_id").get_to(p.ai_studio_id_);
    j.at("project_uuid_id").get_to(p.project_uuid_id_);
    j.at("type").get_to(p.type_);

    if (j.contains("archived")) j.at("archived").get_to(p.archived_);
    if (j.contains("shot_uuid_id")) j.at("shot_uuid_id").get_to(p.shot_uuid_id_);
  }
};
// 统计每人, 每天的 token 消耗量
struct DOODLE_CORE_API person_token {
  DOODLE_BASE_FIELDS();
  chrono::year_month_day date_;
  uuid person_id_;
  // 消耗的 token 数量
  std::int64_t token_consumed_{0};
  // 任务数量
  std::int64_t task_count_{0};
  // 任务参与项目数量
  std::int64_t project_count_{0};
  // 视频类的数量
  std::int64_t video_count_{0};
  // 图片类的数量
  std::int64_t picture_count_{0};

  // to json
  friend void to_json(nlohmann::json& j, const person_token& p) {
    j["id"]            = p.uuid_id_;
    j["date"]          = p.date_;
    j["person_id"]     = p.person_id_;
    j["token_amount"]  = p.token_consumed_;
    j["task_count"]    = p.task_count_;
    j["project_count"] = p.project_count_;
    j["video_count"]   = p.video_count_;
    j["picture_count"] = p.picture_count_;
  }
};
}  // namespace doodle::seedance2