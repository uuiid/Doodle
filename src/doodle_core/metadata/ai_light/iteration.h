//
// AI 灯光：轮次与人工判断（ai_light_iteration）
//
// 一轮 = 一次「生成一批候选 → 派发 → 渲染 → 等美术挑」，一批几个由前端决定。
// 这是会话状态机的推进单位，也是「等人工判断」这个状态的载体。
//
// 关于「等人工判断」放在哪一层（重要）：
//   它被建模成 **iteration/session 状态**，而不是 server_task_info_status。
//   分布式任务是「渲染 6 张图」这件事，它在渲染完成时就终结了；
//   「等美术挑」是会话层面的事，跨在两次任务派发之间，甚至可能跨天。
//   把它塞进 server_task_info_status 会污染那张表的语义，
//   也会让 get_running_task_computer_ids() 的派发判定出错。
//

#pragma once

#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/metadata/ai_light/common.h>
#include <doodle_core/metadata/ai_light/rig.h>
#include <doodle_core/metadata/base.h>

#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace doodle::ai_light {

/// 整轮的推进状态
enum class ai_light_iteration_state {
  /// 本轮候选已生成，尚未派发
  generated,
  /// 已派发给工作机
  dispatched,
  /// 工作机渲染中
  rendering,
  /// 渲染完，等美术判断（人工闸门 2）
  awaiting_judgement,
  /// 已判断
  judged,
};
DOODLE_JSON_SERIALIZE_ENUM_STRICT(
    ai_light_iteration_state, {
                                  {ai_light_iteration_state::generated, "generated"},
                                  {ai_light_iteration_state::dispatched, "dispatched"},
                                  {ai_light_iteration_state::rendering, "rendering"},
                                  {ai_light_iteration_state::awaiting_judgement, "awaiting_judgement"},
                                  {ai_light_iteration_state::judged, "judged"},
                              }
)

/// 美术的判断结论。
/// 注意枚举项名：`continue` 是 C++ 关键字，所以只能叫 continue_iteration。
/// 按本库约定（枚举按 magic_enum 名存 TEXT），JSON 字符串也必须是 "continue_iteration"，
/// 不能是方案 §4.4 里写的 "continue" —— 否则 DB 与 JSON 两处的取值不一致。
enum class judgement_decision {
  /// 定稿，会话结束
  accept,
  /// 选一个候选作为下一轮基准，继续迭代
  continue_iteration,
  /// 全部拒绝，终止会话
  reject_all,
};
DOODLE_JSON_SERIALIZE_ENUM_STRICT(
    judgement_decision, {
                            {judgement_decision::accept, "accept"},
                            {judgement_decision::continue_iteration, "continue_iteration"},
                            {judgement_decision::reject_all, "reject_all"},
                        }
)

/// 人工闸门 2 的记录
struct DOODLE_CORE_API ai_light_judgement_t {
  uuid by_{};
  std::optional<chrono::system_zoned_time> at_{};
  judgement_decision decision_{judgement_decision::continue_iteration};
  /// 被选为下一轮基准（或定稿）的候选。
  /// 方案 §4.4 里这个字段叫 selected_rig_id；本实现统一叫 selected_candidate_id，
  /// 因为它指向的是 ai_light_candidate.uuid_id_（rig 的身份就是候选的身份，没必要两个名字）。
  uuid selected_candidate_id_{};
  /// 美术的自由文本反馈，如「主光再低一点，色温再冷一点」
  ///
  /// 只有这一个反馈字段，**原样留存**：滑块与文本的解释权都归前端，
  /// 服务端不再持有第二份可漂移的真源（`ai_light_orchestrator.h` 的角色降级说明）。
  std::string feedback_text_{};

  friend void to_json(nlohmann::json& j, const ai_light_judgement_t& p) {
    j["by"]                    = p.by_;
    j["at"]                    = p.at_;
    j["decision"]              = p.decision_;
    j["selected_candidate_id"] = p.selected_candidate_id_;
    j["feedback_text"]         = p.feedback_text_;
  }
  friend void from_json(const nlohmann::json& j, ai_light_judgement_t& p) {
    if (j.contains("by")) j.at("by").get_to(p.by_);
    if (j.contains("at")) j.at("at").get_to(p.at_);
    if (j.contains("decision")) j.at("decision").get_to(p.decision_);
    if (j.contains("selected_candidate_id")) j.at("selected_candidate_id").get_to(p.selected_candidate_id_);
    if (j.contains("feedback_text")) j.at("feedback_text").get_to(p.feedback_text_);
  }
};

// ---------------------------------------------------------------------------
// 表实体：ai_light_iteration
// ---------------------------------------------------------------------------

struct DOODLE_CORE_API ai_light_iteration {
  DOODLE_BASE_FIELDS()

  uuid session_id_{};
  /// 从 1 开始。与 session_id 组成唯一键。
  std::int32_t iteration_{1};

  ai_light_iteration_state state_{ai_light_iteration_state::generated};

  /// 派发出去的分布式任务。任务行被删时置空，轮次记录保留。
  uuid server_task_info_id_{};

  std::optional<chrono::system_zoned_time> dispatch_at_{};
  std::optional<chrono::system_zoned_time> render_at_{};

  // 判断人, 判断时间, 判断内容
  uuid judged_by_{};
  std::optional<chrono::system_zoned_time> judged_at_{};
  /// ai_light_judgement_t 的 JSON 形态
  nlohmann::json judgement_{};

  std::string failure_reason_{};

  chrono::system_zoned_time created_at_{chrono::current_zone(), chrono::system_clock::now()};
  chrono::system_zoned_time updated_at_{chrono::current_zone(), chrono::system_clock::now()};

  // --- 类型化访问器 ---
  ai_light_judgement_t get_judgement() const { return json_get_or_default<ai_light_judgement_t>(judgement_); }
  void set_judgement(const ai_light_judgement_t& in_judgement) { judgement_ = in_judgement; }

  /// 记录一次人工判断，并把整轮推进到 judged。
  /// 注意：会话状态（含 4 小时挂起时限）由轮次编排在 ai_light_session 上同步更新，
  /// 本方法只管轮次自己这一行。
  void apply_judgement(const ai_light_judgement_t& in_judgement) {
    set_judgement(in_judgement);
    state_     = ai_light_iteration_state::judged;
    judged_by_ = in_judgement.by_;
    judged_at_ = in_judgement.at_;
  }

  /// 本轮起点 = 上一轮 is_selected 的候选。
  /// 有意不存 base_candidate_id 列 —— 那会造成
  /// iteration → candidate → iteration 的跨表循环外键。
  friend void to_json(nlohmann::json& j, const ai_light_iteration& p) {
    j["id"]                  = p.uuid_id_;
    j["session_id"]          = p.session_id_;
    j["iteration"]           = p.iteration_;
    j["state"]               = p.state_;
    j["server_task_info_id"] = p.server_task_info_id_;
    j["dispatch_at"]         = p.dispatch_at_;
    j["render_at"]           = p.render_at_;
    j["judged_by"]           = p.judged_by_;
    j["judged_at"]           = p.judged_at_;
    j["judgement"]           = p.judgement_;
    j["failure_reason"]      = p.failure_reason_;
    j["created_at"]          = p.created_at_;
    j["updated_at"]          = p.updated_at_;
  }
  friend void from_json(const nlohmann::json& j, ai_light_iteration& p) {
    if (j.contains("id")) j.at("id").get_to(p.uuid_id_);
    if (j.contains("session_id")) j.at("session_id").get_to(p.session_id_);
    if (j.contains("iteration")) j.at("iteration").get_to(p.iteration_);
    if (j.contains("state")) j.at("state").get_to(p.state_);
    if (j.contains("server_task_info_id")) j.at("server_task_info_id").get_to(p.server_task_info_id_);
    if (j.contains("dispatch_at")) j.at("dispatch_at").get_to(p.dispatch_at_);
    if (j.contains("render_at")) j.at("render_at").get_to(p.render_at_);
    if (j.contains("judged_by")) j.at("judged_by").get_to(p.judged_by_);
    if (j.contains("judged_at")) j.at("judged_at").get_to(p.judged_at_);
    if (j.contains("judgement")) j.at("judgement").get_to(p.judgement_);
    if (j.contains("failure_reason")) j.at("failure_reason").get_to(p.failure_reason_);
  }
};

}  // namespace doodle::ai_light
