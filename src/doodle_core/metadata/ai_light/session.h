//
// AI 灯光：会话（ai_light_session）
//
// 一次灯光工作 = 一个会话。同一镜头可以有多个会话（重做灯光），
// 「当前会话」= 最新一个非终态的会话。
//
// 这个表就是方案 §10.2 里的 ai_light_job —— 两者职责完全重叠（都是「一次灯光工作的
// 输入 + 状态」），拆成两张表只会带来同步负担，所以合并。
//
// 有意不做的事：
//   * 不放 current_spec_id / accepted_candidate_id 这类冗余指针 —— 分别由
//     「会话内 revision 最大的 spec」和「is_accepted = true 的候选」查出。
//     本库有 fix_foreign_key_violations / null_dangling_optional_references 两套外键修复
//     机制，循环外键会让「删父行 → 造出新的悬空引用」的分析变复杂，宁可多一次带索引查询。
//   * 不用 updated_at 当业务时钟 —— 任何更新都会刷新它（ORM 的 update 路径会自动刷），
//     所以 4 小时挂起判定单开 judgement_due_at 列。
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

/// 美术超过这个时长不做判断 → 会话挂起（决策 11）。
/// 挂起不是失败：恢复后可以从 suspended 直接回到判断，或继续下一轮。
constexpr auto g_judgement_timeout = std::chrono::hours{4};

/// 会话状态机
enum class ai_light_session_state {
  /// 已创建，提示词已录入，尚未产出 Spec
  draft,
  /// 人工闸门 1：Spec 待美术确认
  awaiting_spec_confirm,
  /// 正在生成候选（LLM + 优化器）
  generating,
  /// 人工闸门 2：6 个候选已渲染完，等美术挑
  awaiting_judgement,
  /// 美术超时未判断 → 挂起
  suspended,
  /// 已定稿
  accepted,
  /// 全部拒绝并终止
  rejected,
  /// 失败
  failed,
};
NLOHMANN_JSON_SERIALIZE_ENUM(
    ai_light_session_state, {
                                {ai_light_session_state::draft, "draft"},
                                {ai_light_session_state::awaiting_spec_confirm, "awaiting_spec_confirm"},
                                {ai_light_session_state::generating, "generating"},
                                {ai_light_session_state::awaiting_judgement, "awaiting_judgement"},
                                {ai_light_session_state::suspended, "suspended"},
                                {ai_light_session_state::accepted, "accepted"},
                                {ai_light_session_state::rejected, "rejected"},
                                {ai_light_session_state::failed, "failed"},
                            }
)

/// 终态判定：终态会话不再接受判断与续轮。
inline bool is_terminal(ai_light_session_state in_state) {
  return in_state == ai_light_session_state::accepted || in_state == ai_light_session_state::rejected ||
         in_state == ai_light_session_state::failed;
}

/// 是否处于「等人工动作」状态。挂起扫描器与前端「待办」列表都用它。
inline bool is_waiting_human(ai_light_session_state in_state) {
  return in_state == ai_light_session_state::awaiting_spec_confirm ||
         in_state == ai_light_session_state::awaiting_judgement || in_state == ai_light_session_state::suspended;
}

struct DOODLE_CORE_API ai_light_session {
  DOODLE_BASE_FIELDS()

  // --- 归属（三个都是业务父行，删父行即删会话）---
  uuid project_id_{};
  uuid task_id_{};   ///< Kitsu 灯光任务，会话锚点
  uuid entity_id_{}; ///< 镜头。Shot 页入口的查询键
  uuid submitter_{};

  std::string title_{};
  /// 人工提示词原文。权威意图，永久留存；LLM 归一化的结果进 ai_light_spec.intent。
  std::string prompt_text_{};

  ai_light_session_state state_{ai_light_session_state::draft};
  /// 当前轮次号，0 = 尚未生成候选。由轮次编排维护；
  /// 也可由 max(ai_light_iteration.iteration) 得出，这里冗余一列是为了让恢复查询免 join。
  std::int32_t current_iteration_{0};

  /// 进入 awaiting_judgement 时写入 now + g_judgement_timeout；判断后清空。
  std::optional<chrono::system_zoned_time> judgement_due_at_{};
  /// 转入 suspended 的时间
  std::optional<chrono::system_zoned_time> suspended_at_{};

  std::string failure_reason_{};

  chrono::system_zoned_time created_at_{chrono::current_zone(), chrono::system_clock::now()};
  chrono::system_zoned_time updated_at_{chrono::current_zone(), chrono::system_clock::now()};

  /// 转入「等美术判断」并起算挂起时限
  void mark_awaiting_judgement() {
    state_          = ai_light_session_state::awaiting_judgement;
    judgement_due_at_ = chrono::system_zoned_time{
        chrono::current_zone(), chrono::system_clock::now() + g_judgement_timeout
    };
    suspended_at_ = std::nullopt;
  }
  /// 判断完成，清掉挂起时限
  void mark_judged(ai_light_session_state in_next) {
    state_            = in_next;
    judgement_due_at_ = std::nullopt;
    suspended_at_     = std::nullopt;
  }
  /// 挂起扫描器调用
  void mark_suspended() {
    state_        = ai_light_session_state::suspended;
    suspended_at_ = chrono::system_zoned_time{chrono::current_zone(), chrono::system_clock::now()};
  }
  /// 挂起扫描器用：是否已过判断时限
  bool is_judgement_overdue(const chrono::system_zoned_time& in_now) const {
    return state_ == ai_light_session_state::awaiting_judgement && judgement_due_at_ &&
           in_now.get_sys_time() > judgement_due_at_->get_sys_time();
  }

  friend void to_json(nlohmann::json& j, const ai_light_session& p) {
    j["id"]               = p.uuid_id_;
    j["project_id"]       = p.project_id_;
    j["task_id"]          = p.task_id_;
    j["entity_id"]        = p.entity_id_;
    j["submitter"]        = p.submitter_;
    j["title"]            = p.title_;
    j["prompt_text"]      = p.prompt_text_;
    j["state"]            = p.state_;
    j["current_iteration"] = p.current_iteration_;
    j["judgement_due_at"] = p.judgement_due_at_;
    j["suspended_at"]     = p.suspended_at_;
    j["failure_reason"]   = p.failure_reason_;
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
    if (j.contains("state")) j.at("state").get_to(p.state_);
    if (j.contains("current_iteration")) j.at("current_iteration").get_to(p.current_iteration_);
    if (j.contains("judgement_due_at")) j.at("judgement_due_at").get_to(p.judgement_due_at_);
    if (j.contains("suspended_at")) j.at("suspended_at").get_to(p.suspended_at_);
    if (j.contains("failure_reason")) j.at("failure_reason").get_to(p.failure_reason_);
  }
};

}  // namespace doodle::ai_light
