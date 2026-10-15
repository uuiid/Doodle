#pragma once
#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/metadata/base.h>

#include <chrono>
#include <magic_enum/magic_enum.hpp>
#include <string>
#include <string_view>

namespace doodle::seedance2 {
/**
preparing: 准备提交
queued：排队中。
running：任务运行中。
cancelled：取消任务，取消状态24h自动删除（只支持排队中状态的任务被取消）。
succeeded： 任务成功。（如发送失败，即5秒内没有接收到成功发送的信息，回调三次）
failed：任务失败。（如发送失败，即5秒内没有接收到成功发送的信息，回调三次）
violation：违规。如内容审核未通过、账号配额或免费额度受限。是否扣费取决于模型定价的 charge_on_violation。
expired：任务超时，即任务处于运行中或排队中状态超过过期时间。可通过 execution_expires_after 字段设置过期时间。
*/
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

// 任务类别 图片,视频等
//
// 注意：新值只能**追加在末尾**。枚举在库里以 magic_enum 推出来的**名字串**存储
// (sqlite_statement.h:270-275)，读取时用 enum_cast 反查、认不出就抛
// (sqlite_statement.h:283-284)。所以：
//   * 重排或改名会让历史行读不出来；
//   * **必须先升服务端**再产生 "inference" 这类行, 但理由比"读不出来"更糟:
//     - 库这条路径(老服务端)读到认不出的名字串会抛(至少不是静默错)；
//     - JSON 这条路径(老服务端收到新前端的 `"type":"inference"`)会**静默变成 picture**
//       —— nlohmann 的 SERIALIZE_ENUM 认不出就落到第一个枚举值
//       (nlohmann/detail/macro_scope.hpp:247)。本文件已用 DOODLE_JSON_SERIALIZE_ENUM_STRICT
//       把这个静默回落堵住, 但**老二进制里没有这道闸**, 所以顺序仍然必须是「先升服务端」。
//   * JSON 名必须与 magic_enum 推出来的名字一致（都是 inference）：同一个宏把映射显式
//     列了出来, 两边不一致会在运行期被挡住。
enum class DOODLE_CORE_API task_type {
  picture,
  video,
  /// 同步推理（火山方舟 Responses API）。它不是异步生成任务：run_task 一次调用
  /// 就拿到结果，任务行直接落终态（见 seedance2/task.cpp 的 run_inference_task）。
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
// 后端分类
enum class task_backend {
  seedance2,          // 豆包/火山方舟 Seedance2
  transfer_station,   // 中转站
  apipy_image,        // API易(apiyi.com) 图片后端 —— 同步单次调用, 无 task_id
};
DOODLE_JSON_SERIALIZE_ENUM_STRICT(
    task_backend,
    {{task_backend::seedance2, "seedance2"},
     {task_backend::transfer_station, "transfer_station"},
     {task_backend::apipy_image, "apipy_image"}}
);

struct DOODLE_CORE_API task {
  DOODLE_BASE_FIELDS();
  uuid user_id_;
  task_status status_{task_status::preparing};
  task_type type_{task_type::video};
  task_backend backend_{task_backend::seedance2};
  nlohmann::json data_request_;
  std::string text_prompt_;
  uuid preview_file_;  // 对应 ai_preview_file
  std::string file_extension_;
  nlohmann::json data_response_;
  uuid ai_studio_id_;

  std::string task_id_;  // 任务ID，外部唯一标识
  chrono::system_zoned_time created_at_{chrono::current_zone(), chrono::system_clock::now()};
  chrono::system_zoned_time ended_at_{chrono::current_zone(), chrono::system_clock::now()};

  uuid project_uuid_id_;  // 内部使用的UUID，对应项目中的uuid_id_
  /// 任务计费口径:
  ///   * 生成类任务 = 外部服务返回的 completion_tokens(提交时先按 default_consumed_tokens 预扣, 结算时退补);
  ///   * 推理任务(type_ == inference) = 方舟响应的 **total_tokens**(输入 token 才是成本大头),
  ///     由 run_inference_task 当场结算, **必须显式赋值** —— 否则会沿用下面的默认值, 污染配额与统计。
  std::int64_t completion_tokens_{doodle_config::g_max_task_completion_tokens};  // 默认上限, 见 g_max_task_completion_tokens
  uuid ai_generate_entity_id_;  // 内部使用的UUID，对应ai_generate_entity中的uuid_id_
  uuid subproject_id_;          // 对应 subproject 的 uuid_id_

  /// 外键 → ai_light_session.uuid_id_。AI 灯光的推理任务（type_ == inference）指向它所属的会话。
  /// 可空：非 AI 灯光来源的推理任务该字段为 nil。
  uuid ai_light_session_id_;

  std::int64_t retry_count_{0};  // 重试次数
  // 归档
  bool archived_;
  // to json
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
    // type / backend 走 DOODLE_JSON_SERIALIZE_ENUM_STRICT 生成的 from_json:
    // 认不出的取值当场抛 out_of_range.410, 不会静默变成一个图片生成任务
    j.at("type").get_to(p.type_);
    j.at("backend").get_to(p.backend_);
    j.at("ai_generate_entity_id").get_to(p.ai_generate_entity_id_);

    if (j.contains("archived")) j.at("archived").get_to(p.archived_);
    if (j.contains("ai_light_session_id")) j.at("ai_light_session_id").get_to(p.ai_light_session_id_);
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
  // 推理类的数量（AI 灯光等同步推理任务）
  std::int64_t inference_count_{0};

  // to json
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
// task 任务和之前任务的相似度, 用于判断是否重复提交任务
struct DOODLE_CORE_API task_similarity {
  DOODLE_BASE_FIELDS();
  uuid task_id_;
  uuid similar_task_id_;
  double similarity_{0.0};

  // to json
  friend void to_json(nlohmann::json& j, const task_similarity& p) {
    j["id"]              = p.uuid_id_;
    j["task_id"]         = p.task_id_;
    j["similar_task_id"] = p.similar_task_id_;
    j["similarity"]      = p.similarity_;
  }
};

}  // namespace doodle::seedance2