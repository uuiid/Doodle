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
//   * 不放 accepted_candidate_id 这类冗余指针 —— 由「is_accepted = true 的候选」查出。
//     本库有 fix_foreign_key_violations / null_dangling_optional_references 两套外键修复
//     机制，循环外键会让「删父行 → 造出新的悬空引用」的分析变复杂，宁可多一次带索引查询。
//     （同段的 current_spec_id 已随 ai_light_spec 表一起消失。）
//   * 不用 updated_at 当业务时钟 —— 任何更新都会刷新它（ORM 的 update 路径会自动刷），
//     所以 4 小时过期判定单开 judgement_due_at 列。
//   * 不把「过期」做成一个状态 —— 它是 judgement_due_at 的纯派生事实，
//     物化成状态只会让「等判断」这条唯一的判断入口被自己关掉。见 g_judgement_timeout。
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

/// 美术超过这个时长不做判断 → 会话**只是被标记为「已过期」，状态不变**（决策 12）。
///
/// 为什么过期不是一个独立状态，而是一个实时派生的标志：
/// 它是 `state == awaiting_judgement && now > judgement_due_at_` 的纯函数，
/// `is_judgement_overdue()` 随时算得出来，读模型也把它带出去了（`is_judgement_overdue_`）。
///
/// 这一点必须守住：`apply_judgement` 只接受 `awaiting_judgement`，若把过器物化成一个
/// 新状态，就没有任何路径能回到 `awaiting_judgement`（`mark_awaiting_judgement()` 只在
/// 渲染回填里调用，而渲染早已完成）—— 美术超时回来时，**已经渲染好的 6 个候选再也判断不了**，
/// 只能走 `/spec` 重出方案，白扔一整轮渲染。同理，`is_judgement_overdue()` 也不能要求
/// 状态是 awaiting_judgement，否则恰恰在「真的过期了」那一刻过期信号会消失。
///
/// 另外「挂起能省资源」在这里是空头支票：会话进入 `awaiting_judgement` 的前提就是渲染已完成、
/// worker 已归还，此时没有农场槽位、没有 UE 授权、没有 GPU 被占，6 张 PNG 只是躺在磁盘上。
constexpr auto g_judgement_timeout = std::chrono::hours{4};

/// 会话状态机 —— **没有终态**。
///
/// 美术对同一个镜头的灯光可能是**无限轮**的调整（今天定稿，明天导演改意见，回来接着调），
/// 所以这里不设终态：一旦有终态，`check_session_mutable()` 会把传图、删图、重出 Spec
/// 全部挡成 400，那段工作就只能新建一个会话重来 —— 输入、Spec、历史轮次全部作废。
///
/// 于是模型是「会话是一个长期工作台」：
///   * 定稿**不是终点**，而是**可移动的单指针**（`ai_light_candidate.accepted_session_id`
///     上的唯一索引仍然保证「当前定稿」只有一个，再次定稿会顶掉上一次）；
///   * `reject_all` **不终止会话**，它只表示「这一轮全部不要」，会话照常可以再出一轮；
///   * 失败没有独立状态，`failure_reason_` 是唯一记录（`advance_session` 的接缝失败走回滚）。
///
/// 一轮只能判断一次（`apply_judgement` 的 `iteration.state_ != judged` 闸门），
/// 所以「再来一轮」必须经 `POST /iterate` —— 这是有意的，见 §15.10。
enum class ai_light_session_state {
  /// 已创建，提示词已录入，尚未产出 Spec
  draft,
  /// 人工闸门 1：Spec 待美术确认
  awaiting_spec_confirm,
  /// 正在生成候选（LLM + 优化器）。**判断之后也回到这里**，所以它同时表示
  /// 「可以再出一轮」——`POST /iterate` 只在这个状态下可用。
  generating,
  /// 人工闸门 2：6 个候选已渲染完，等美术挑。
  /// 美术迟迟不来**不会**改变这个状态，只会让 is_judgement_overdue() 变成 true。
  awaiting_judgement,
};
DOODLE_JSON_SERIALIZE_ENUM_STRICT(
    ai_light_session_state, {
                                {ai_light_session_state::draft, "draft"},
                                {ai_light_session_state::awaiting_spec_confirm, "awaiting_spec_confirm"},
                                {ai_light_session_state::generating, "generating"},
                                {ai_light_session_state::awaiting_judgement, "awaiting_judgement"},
                            }
)

/// 是否处于「等人工动作」状态。前端「待办」列表用它。
///
/// 注意 overdue 的会话仍然算「等人工动作」：过期不代表可以不管了，
/// 只是提醒前端该催了。要区分「刚进入等判断」和「已经拖太久了」，
/// 用读模型里的 is_judgement_overdue_。
inline bool is_waiting_human(ai_light_session_state in_state) {
  return in_state == ai_light_session_state::awaiting_spec_confirm ||
         in_state == ai_light_session_state::awaiting_judgement;
}

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

  ai_light_session_state state_{ai_light_session_state::draft};
  /// 当前轮次号，0 = 尚未生成候选。由轮次编排维护；
  /// 也可由 max(ai_light_iteration.iteration) 得出，这里冗余一列是为了让恢复查询免 join。
  std::int32_t current_iteration_{0};

  /// 进入 awaiting_judgement 时写入 now + g_judgement_timeout；判断后清空。
  /// 这是「过期」的唯一依据 —— 过期不落成状态，由 is_judgement_overdue() 实时算。
  std::optional<chrono::system_zoned_time> judgement_due_at_{};

  std::string failure_reason_{};

  chrono::system_zoned_time created_at_{chrono::current_zone(), chrono::system_clock::now()};
  chrono::system_zoned_time updated_at_{chrono::current_zone(), chrono::system_clock::now()};

  /// 转入「等美术判断」并起算过期时限
  void mark_awaiting_judgement() {
    state_          = ai_light_session_state::awaiting_judgement;
    judgement_due_at_ = chrono::system_zoned_time{
        chrono::current_zone(), chrono::system_clock::now() + g_judgement_timeout
    };
  }
  /// 判断完成，清掉过期时限
  void mark_judged(ai_light_session_state in_next) {
    state_            = in_next;
    judgement_due_at_ = std::nullopt;
  }
  /// 是否已过判断时限。**纯派生**，不改状态 —— 见 g_judgement_timeout 的说明。
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
    if (j.contains("failure_reason")) j.at("failure_reason").get_to(p.failure_reason_);
  }
};

}  // namespace doodle::ai_light
