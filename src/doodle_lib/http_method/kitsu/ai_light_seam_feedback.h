//
// AI 灯光 · 接缝 3：自由文本反馈 → 结构化轴调整
// （设计见 docs/ue_ai_lighting_plan.md §15.9）
//
// 与前两条接缝同样的分法：纯函数组信封/映射 + 一次真实网络调用。
//
// 伞形头 ai_light_orchestrator.h 会 include 本头 —— 对外仍然只需包含伞形头一个头。
//

#pragma once

#include <doodle_core/metadata/ai_light/candidate.h>
#include <doodle_core/metadata/ai_light/iteration.h>
#include <doodle_core/metadata/ai_light/spec.h>

#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_lib/http_client/seedance2_inference_request.h>

#include "ai_light_seam_common.h"

#include <boost/asio/awaitable.hpp>
#include <nlohmann/json.hpp>

#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace doodle::ai_light {

// ===========================================================================
// 接缝 3：自由文本反馈 → 结构化轴调整
// ===========================================================================

struct DOODLELIB_API feedback_structuring_request_t : public seam_request_base_t {
  /// 被判断的这一轮。进 sd2::task.ai_light_iteration_id_，用于反查「哪一次推理结构化了
  /// 哪一轮的反馈」。落 iteration 行本身仍由调用方做（契约 2）。
  uuid iteration_id_{};
  /// 从 1 开始
  std::int32_t iteration_{0};
  /// 美术的原话。**逐字进提示词** —— 改写它等于替美术重新表述需求。
  std::string feedback_text_{};
  /// 这次判断的决定。只有 continue_iteration 才值得调本接缝：
  /// accept / reject_all 会终止会话，结构化出来的轴没有下一轮可以作用（§15.9 决定 D16）。
  judgement_decision decision_{judgement_decision::continue_iteration};
  /// 美术选中的候选（可为 nil）。**文本最可能指的就是它** —— 提示词会把它的完整 rig
  /// 摊给模型看（「第 3 个太黄了」需要一个明确的「第 3 个」）。
  uuid selected_candidate_id_{};
  /// 当前轮 Spec。调用方加载（与另两条接缝一致：接缝拿的是纯值输入，可离线测）。
  lighting_spec_t spec_{};
  /// 本轮的全部候选。OFAT 不变量（§15.8）保证「基线 rig + 各候选的轴档位」足以描述
  /// 全部 6 张图，所以提示词只需展开第 0 个。
  std::vector<ai_light_candidate> candidates_{};
  /// 美术已经用滑块给出的方向（决策 5：滑块是默认通道）。这些轴不必再让模型给，
  /// 调用方合并时也不会被文本覆盖 —— 显式拖出的数值比自然语言精确。
  std::map<std::string, double> slider_axes_{};
};

/// 「主光再低一点，色温再冷一点」这类自由文本 → 轴调整。
///
/// 返回的每个 axis_step_t **恰好一个 steps_ 元素**：judge 只传「下一轮的中心值」，
/// 完整档位安排由下一轮**前端**自己排的轴计划决定（服务端不再展开候选）。空 vector 是合法结果
/// （「再调调看」这类没有方向的反馈）—— 不是错误。
///
/// in_sql 用来解析 `operator_id_ → person.ai_studio_id_ → ai_studio`（取推理密钥）
/// 并交给 run_inference_task 落任务行与扣费（契约 2 的受控例外）。
///
/// 失败一律抛 doodle_error（形状违规 → bad_gateway；人员/工作室缺失 → bad_request）。
/// **语义无效的轴不抛错，在源头丢弃**：none / 0 / 未知轴名 / key_fill_ratio <= 0
/// 若传下去，下一轮会以「轴无实质变化」抛 502，那时病根与表现隔了一次人工判断。
DOODLELIB_API boost::asio::awaitable<std::vector<axis_step_t>> structure_feedback(
    sqlite_database& in_sql, const feedback_structuring_request_t& in_request
);

// ---------------------------------------------------------------------------
// 接缝 3 的离线可测部件
// ---------------------------------------------------------------------------

/// 系统指令：翻译环节的角色 + **共用**的轴值语义段（与 apply_axis_step 同一份口径）
/// + 只给方向 / 最多 2 条轴 / 别为凑数编一条 等约束。
DOODLELIB_API std::string build_feedback_instructions();

/// 结构化输出契约：{understanding, axes[{axis, value, reason}]}。
/// axis 的 enum 走 json_val::schema_enum<variant_axis_kind>(variant_axis_kind::none)
/// （剔除 none，与接缝 2 的轴枚举是**同一份**建造器）；axes.maxItems == 2，
/// **刻意不设 minItems**（空数组是「美术没给出方向」的合法表达）。
DOODLELIB_API nlohmann::json build_feedback_json_schema();

/// 用户消息正文：美术原话 + 决定 + Spec 目标 + 本轮 6 个候选的对照结构 +
/// 选中候选的完整 rig + 当前 key:fill 比 + 滑块已给的方向。
DOODLELIB_API std::string build_feedback_input_text(const feedback_structuring_request_t& in_request);

/// 归一化推理信封。与另两条接缝一致：不发 images / image_detail / max_output_tokens / thinking。
DOODLELIB_API http::seedance2::inference_request_t build_feedback_request(
    const feedback_structuring_request_t& in_request
);

/// LLM 结果 → 轴方向。形状违规抛 bad_gateway；语义无效的轴**静默丢弃**（见结构体注释）。
/// 本函数**刻意不记日志**：单测宿主没有 core_set::ctx_ptr 也没有 HTTP logger，纯函数必须
/// 能在单测里直接调用 —— 「哪个轴被丢了、为什么」由调用方在接缝外按需补日志。
DOODLELIB_API std::vector<axis_step_t> map_feedback_result(const nlohmann::json& in_result_json);

// key_fill_ratio_of 定义在 ai_light_rig_ops.h（它在接缝 3 的提示词里被引用，但本身是纯 rig 读取）。

}  // namespace doodle::ai_light
