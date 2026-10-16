//
// AI 灯光：会话（ai_light_session）
//
// 一次灯光工作 = 一个会话。同一镜头可以有多个会话（重做灯光），
// 「当前会话」= 最新一个非终态的会话。
//
// 这个表就是方案 §10.2 里的 ai_light_job —— 两者职责完全重叠（都是「一次灯光工作的
// 输入 + 状态」），拆成两张表只会带来同步负担，所以合并。
//
// 会话**不自持状态机** —— 推进状态与过期时限都在 ai_light_iteration 上
// （见 iteration.h 的 is_waiting_human / is_judgement_overdue）。
// 会话只是输入与归属的容器：一个镜头的一次灯光工作的提示词、参考图、以及若干轮次。
//

#pragma once

#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/metadata/ai_light/common.h>
#include <doodle_core/metadata/base.h>

#include <chrono>
#include <cstdint>
#include <optional>
#include <string>

namespace doodle::ai_light {

struct DOODLE_CORE_API ai_light_session {
  DOODLE_BASE_FIELDS()

  // --- 归属（三个都是业务父行，删父行即删会话）---
  uuid project_id_{};
  uuid task_id_{};   ///< Kitsu 灯光任务，会话锚点
  uuid entity_id_{}; ///< 镜头。Shot 页入口的查询键
  uuid submitter_{};

  std::string title_{};
  /// 人工提示词原文。权威意图，永久留存。后端不再做 LLM 归一化（曾进 ai_light_spec.intent）。
  std::string prompt_text_{};

  chrono::system_zoned_time created_at_{chrono::current_zone(), chrono::system_clock::now()};
  chrono::system_zoned_time updated_at_{chrono::current_zone(), chrono::system_clock::now()};

  friend void to_json(nlohmann::json& j, const ai_light_session& p) {
    j["id"]               = p.uuid_id_;
    j["project_id"]       = p.project_id_;
    j["task_id"]          = p.task_id_;
    j["entity_id"]        = p.entity_id_;
    j["submitter"]        = p.submitter_;
    j["title"]            = p.title_;
    j["prompt_text"]      = p.prompt_text_;
    j["created_at"]       = p.created_at_;
    j["updated_at"]       = p.updated_at_;
  }
  friend void from_json(const nlohmann::json& j, ai_light_session& p) {
    if (j.contains("id")) j.at("id").get_to(p.uuid_id_);
    if (j.contains("project_id")) j.at("project_id").get_to(p.project_id_);
    if (j.contains("task_id")) j.at("task_id").get_to(p.task_id_);
    if (j.contains("entity_id")) j.at("entity_id").get_to(p.entity_id_);
    if (j.contains("submitter")) j.at("submitter").get_to(p.submitter_);
    if (j.contains("title")) j.at("title").get_to(p.title_);
    if (j.contains("prompt_text")) j.at("prompt_text").get_to(p.prompt_text_);
  }
};

}  // namespace doodle::ai_light
