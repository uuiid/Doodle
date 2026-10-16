//
// AI 灯光：读模型（不落表）
//
// 这里只有「给前端看的聚合体」，没有表实体 —— 表实体在 session/reference/spec/
// iteration/candidate/preset 各自的头里。
//
// 为什么单独一个文件：会话详情是六张表的聚合，而 session.h 目前不依赖任何其它
// ai_light 头。把聚合体塞进 session.h 会让它被迫包含 spec/iteration/candidate，
// 而 candidate.h 又包含 rig.h …… 于是所有头文件互相缠在一起。读模型天然位于
// 六个表实体之上，单独一层最干净，也让「响应形状」有唯一权威定义、可脱开 HTTP 单测。
//

#pragma once

#include <doodle_core/metadata/ai_light/candidate.h>
#include <doodle_core/metadata/ai_light/iteration.h>
#include <doodle_core/metadata/ai_light/reference.h>
#include <doodle_core/metadata/ai_light/session.h>
#include <doodle_core/metadata/ai_light/spec.h>

#include <algorithm>
#include <cstdint>
#include <optional>
#include <vector>

namespace doodle::ai_light {

/// 会话详情：一个镜头的一次灯光工作的全貌。
///
/// 前端只调这一个接口就能渲染整个工作台（参考图、当前目标、每轮候选、
/// 上一轮判断、当前该谁动）。刻意不做分页与懒加载 —— 一次会话的候选量级是
/// 6 × 轮数（实际收敛在 3 轮内），总共几十行，一次取完比多次往返简单得多。
struct DOODLE_CORE_API ai_light_session_detail_t {
  ai_light_session session_{};

  /// 参考图，按 created_at 升序（= 上传先后顺序）
  std::vector<ai_light_reference> references_{};

  /// 轮次，iteration 升序
  std::vector<ai_light_iteration> iterations_{};
  /// 候选，跨轮，按（轮次, 候选序号）升序。每轮的个数由前端决定，不固定。
  std::vector<ai_light_candidate> candidates_{};

  // --- 两个派生提示位，省得前端自己算状态机 ---
  /// 是否在等人工动作（= ai_light::is_waiting_human(**最新一轮**)，无轮次则 false）
  bool is_waiting_human_{false};
  /// 是否已过 4 小时判断时限。前端据此把待办高亮成「快挂起了」。
  /// 与挂起扫描器用同一个判据（ai_light::is_judgement_overdue(最新一轮, now)），
  /// 所以前端显示「已超时」时扫描器一定也认为该挂起，不会出现两边不一致。
  bool is_judgement_overdue_{false};

  /// 前端渲染进度条要的「每轮几个候选」。
  /// 候选数由前端决定，所以取**最近一轮实际落了几个候选**作为当前这一轮的数字。
  std::int32_t candidate_count_per_iteration() const {
    if (candidates_.empty()) return 0;
    // candidates_ 按（轮次, 候选序号）升序，所以最后一个就是最近一轮的最后一个候选。
    const auto& l_latest_id = candidates_.back().iteration_id_;
    return static_cast<std::int32_t>(
        std::count_if(
            candidates_.begin(), candidates_.end(),
            [&l_latest_id](const ai_light_candidate& in_c) { return in_c.iteration_id_ == l_latest_id; }
        )
    );
  }

  friend void to_json(nlohmann::json& j, const ai_light_session_detail_t& p) {
    j["session"]            = p.session_;
    j["references"]         = p.references_;
    j["iterations"]         = p.iterations_;
    j["candidates"]         = p.candidates_;
    j["is_waiting_human"]   = p.is_waiting_human_;
    j["is_judgement_overdue"] = p.is_judgement_overdue_;
    // 前端渲染进度条要用，省得自己数。
    // 候选数由前端决定，所以这里报**当前会话的实际情况**：
    // 用最近一轮实际落了几个候选；还没有轮次时为 0。
    j["candidate_count_per_iteration"] = p.candidate_count_per_iteration();
  }
  friend void from_json(const nlohmann::json& j, ai_light_session_detail_t& p) {
    if (j.contains("session")) j.at("session").get_to(p.session_);
    if (j.contains("references")) j.at("references").get_to(p.references_);
    if (j.contains("iterations")) j.at("iterations").get_to(p.iterations_);
    if (j.contains("candidates")) j.at("candidates").get_to(p.candidates_);
    if (j.contains("is_waiting_human")) j.at("is_waiting_human").get_to(p.is_waiting_human_);
    if (j.contains("is_judgement_overdue")) j.at("is_judgement_overdue").get_to(p.is_judgement_overdue_);
  }
};

}  // namespace doodle::ai_light
