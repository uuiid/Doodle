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

#include <chrono>
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

/// 美术超过这个时长不做判断 → 轮次**只是被标记为「已过期」，状态不变**（决策 12）。
///
/// 为什么过期不是一个独立状态，而是一个实时派生的标志：
/// 它是 `state == awaiting_judgement && now > render_at_ + g_judgement_timeout` 的纯函数，
/// `is_judgement_overdue()` 随时算得出来，读模型也把它带出去了（`is_judgement_overdue_`）。
///
/// 这一点必须守住：`judge_route::post` 的判断闸门只接受 `awaiting_judgement`，若把过器物化成一个
/// 新状态，就没有任何路径能回到 `awaiting_judgement`（渲染回填是唯一的写侧，而渲染早已完成）
/// —— 美术超时回来时，**已经渲染好的候选再也判断不了**。
///
/// 另外「挂起能省资源」在这里是空头支票：轮次进入 `awaiting_judgement` 的前提就是渲染已完成、
/// worker 已归还，此时没有农场槽位、没有 UE 授权、没有 GPU 被占，6 张 PNG 只是躺在磁盘上。
constexpr auto g_judgement_timeout = std::chrono::hours{4};

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

  // 判断人, 判断时间
  uuid judged_by_{};
  std::optional<chrono::system_zoned_time> judged_at_{};
  /// 美术的自由文本反馈，如「主光再低一点，色温再冷一点」
  std::string feedback_text_{};

  std::string failure_reason_{};
  // 当前轮的灯光方案
  nlohmann::json rigs_{};

  chrono::system_zoned_time created_at_{chrono::current_zone(), chrono::system_clock::now()};
  chrono::system_zoned_time updated_at_{chrono::current_zone(), chrono::system_clock::now()};

  lighting_rig_set_t get_rigs() const { return rigs_.get<lighting_rig_set_t>(); }
  void set_rigs(const lighting_rig_set_t& in_rigs) { rigs_ = in_rigs; }

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
    j["feedback_text"]       = p.feedback_text_;
    j["failure_reason"]      = p.failure_reason_;
    j["rigs"]                = p.rigs_;
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
    if (j.contains("feedback_text")) j.at("feedback_text").get_to(p.feedback_text_);
    if (j.contains("failure_reason")) j.at("failure_reason").get_to(p.failure_reason_);
    if (j.contains("rigs")) j.at("rigs").get_to(p.rigs_);
  }
};

/// 这一轮是否在等美术判断。前端「待办」列表用它 —— 会话层面取**最新一轮**再问本谓词。
///
/// 注意 overdue 的轮次仍然算「等人工动作」：过期不代表可以不管了，
/// 只是提醒前端该催了。要区分「刚进入等判断」和「已经拖太久了」，
/// 用读模型里的 is_judgement_overdue_。
///
/// 保留成谓词形状：前端读的是 `is_waiting_human_` 这个**标志**而不是状态枚举，
/// 将来再加闸门时不必改读模型。
inline bool is_waiting_human(const ai_light_iteration& in_iteration) {
  return in_iteration.state_ == ai_light_iteration_state::awaiting_judgement;
}

/// 是否已过判断时限。**纯派生**，不改状态 —— 见 g_judgement_timeout 的说明。
///
/// 基准时刻是 `render_at_`（转入 awaiting_judgement 的同一事务里写入），不是会话的
/// 任何时钟：判断之后 state_ 不再是 awaiting_judgement，过期信号自然消失。
inline bool is_judgement_overdue(const ai_light_iteration& in_iteration, const chrono::system_zoned_time& in_now) {
  return is_waiting_human(in_iteration) && in_iteration.render_at_.has_value() &&
         in_now.get_sys_time() > in_iteration.render_at_->get_sys_time() + g_judgement_timeout;
}

}  // namespace doodle::ai_light
