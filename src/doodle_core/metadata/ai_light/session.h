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
/// 这一点必须守住：`judge_route::post` 的判断闸门只接受 `awaiting_judgement`，若把过器物化成一个
/// 新状态，就没有任何路径能回到 `awaiting_judgement`（`mark_awaiting_judgement()` 只在
/// 渲染回填里调用，而渲染早已完成）—— 美术超时回来时，**已经渲染好的候选再也判断不了**，
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
///   * `reject_all` **不终止会话**，它只表示「这一轮全部不要」，会话照常可以再出一轮；
///   * 失败没有独立状态，失败只留日志（状态与候选行原地不动）。
///
/// 一轮只能判断一次（`judge_route::post` 的 `iteration.state_ != judged` 闸门），
/// 所以「再来一轮」必须经 `POST /iterate` —— 这是有意的，见 §15.10。
enum class ai_light_session_state {
  /// 已创建，提示词已录入，尚未产出 Spec
  ///
  /// **可提交候选**（与 generating 并列，见 can_submit_candidates）。这一条是为
  /// 「取消人工闸门 1」补上的：闸门 1 删除后，只要有参考图就能直接干活，
  /// 而建会话时就已有提示词、上传首张参考图的返回里也已经是 generating。
  draft,
  /// 正在生成候选（LLM + 优化器）。**判断之后也回到这里**，所以它同时表示
  /// 「可以再出一轮」——`POST /iterate` 只在这个状态下可用。
  generating,
  /// 人工闸门 2：本轮候选已渲染完，等美术挑。
  /// 美术迟迟不来**不会**改变这个状态，只会让 is_judgement_overdue() 变成 true。
  awaiting_judgement,
};
DOODLE_JSON_SERIALIZE_ENUM_STRICT(
    ai_light_session_state, {
                                {ai_light_session_state::draft, "draft"},
                                {ai_light_session_state::generating, "generating"},
                                {ai_light_session_state::awaiting_judgement, "awaiting_judgement"},
                            }
)

/// 曾经的 `awaiting_spec_confirm`（人工闸门 1：Spec 待美术确认）已删除。
///
/// 删除理由：闸门 1 的语义是「人工确认**服务端**蒸馏出的输出版本」，而 Spec 蒸馏早已
/// 归前端（见 ai_light_orchestrator.h 的角色降级说明）—— 服务端手上没有可确认的对象。
/// 该状态在全仓**没有任何写侧**（没有一处把会话置为它），它自己写出的 generating
/// 又只被 `/iteration` 提交接受，于是承载它的 `/spec/confirm` 路由只能成功一次。
/// 现在只剩**一个**人工闸门：awaiting_judgement（/judge）。

/// 是否处于「等人工动作」状态。前端「待办」列表用它。
///
/// 注意 overdue 的会话仍然算「等人工动作」：过期不代表可以不管了，
/// 只是提醒前端该催了。要区分「刚进入等判断」和「已经拖太久了」，
/// 用读模型里的 is_judgement_overdue_。
///
/// 人工闸门 1 取消后，`awaiting_spec_confirm` 分支随之删除 —— 现在唯一等人工的
/// 状态就是 awaiting_judgement，但本函数保留成谓词形状：前端读的是
/// `is_waiting_human_` 这个**标志**而不是状态枚举，将来再加闸门时不必改读模型。
inline bool is_waiting_human(ai_light_session_state in_state) {
  return in_state == ai_light_session_state::awaiting_judgement;
}

/// 是否可以向会话提交一轮候选。
///
/// 两个状态都放行，理由不同：
///   * `draft` —— 建会话后即可提交。取消人工闸门 1 之后，服务端不再要求「先确认
///     输出版本」；只要提示词与参考图齐了就能干活。仍要求**存在参考图**这一点
///     由 /iteration 提交自己校验（参考图是候选生成的前提）。
///   * `generating` —— 正常路径（上传参考图后即为此状态），以及判断之后回到的「再出一轮」。
///
/// `awaiting_judgement` 明确**不**放行：上一轮候选还没被判断，此时提交会让它们
/// 变成孤儿（见 ai_light_submit.cpp 的同名闸门）。
///
/// 与 is_waiting_human 一样放在枚举旁边：判定条件与状态枚举同处一文件，
/// 将来增删状态时改枚举的人就在隔壁看得到这个谓词。
inline bool can_submit_candidates(ai_light_session_state in_state) {
  return in_state == ai_light_session_state::draft || in_state == ai_light_session_state::generating;
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
  }
};

}  // namespace doodle::ai_light
