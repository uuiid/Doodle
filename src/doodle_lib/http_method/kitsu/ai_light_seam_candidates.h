//
// AI 灯光 · 接缝 2：基线 rig + Spec → 本轮 6 个候选
// （设计见 docs/ue_ai_lighting_plan.md §15.8）
//
// 与接缝 1 同样的分法：纯函数组信封/映射 + 一次真实网络调用。约定也相同 ——
// 必须定义在命名空间 doodle::ai_light 内（匿名命名空间 = 内部链接 = 库外 LNK2019）。
//
// 与接缝 1 的关键差别：**展开器是确定性的**（§8.5 第 1 条）。模型只输出「一套基线灯
// 集合 + 一份轴计划」，6 个候选由 expand_candidates() 机械展开，所以
// 「同一基线 + 同一轴计划 → 同一组 rig」可以脱网复现与断言。
//
// 伞形头 ai_light_orchestrator.h 会 include 本头 —— 对外仍然只需包含伞形头一个头。
//

#pragma once

#include <doodle_core/metadata/ai_light/iteration.h>
#include <doodle_core/metadata/ai_light/preset.h>
#include <doodle_core/metadata/ai_light/rig.h>
#include <doodle_core/metadata/ai_light/spec.h>

#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_lib/http_client/seedance2_inference_request.h>

#include "ai_light_seam_common.h"

#include <boost/asio/awaitable.hpp>
#include <nlohmann/json.hpp>

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace doodle::ai_light {

// ===========================================================================
// 接缝 2：基线 rig + Spec → 本轮 6 个候选
// ===========================================================================

struct DOODLELIB_API candidate_generation_request_t : public seam_request_base_t {
  /// 本轮 iteration 的 uuid。调用方在调用**之前**铸造，好让接缝把它写进
  /// sd2::task.ai_light_iteration_id_（可空 FK），从而能反查「哪一次推理生成了这一轮候选」。
  /// 落 iteration 行本身仍由调用方在接缝返回后做（契约 2：接缝不写 ai_light_* 表）。
  uuid iteration_id_{};
  /// 从 1 开始
  std::int32_t iteration_{1};
  lighting_spec_t spec_{};
  /// warm start 起点 = 上一轮被选中的候选；首轮为空（从预设库起手）。
  ///
  /// 调用方只在「上一轮 iteration.spec_id_ 就是当前 Spec」时才填它 —— Spec 升版后上一轮
  /// 的 rig 解的是旧目标，拿它当起点等于让新一轮从旧目标出发。
  std::optional<lighting_rig_t> base_rig_{};
  /// 上一轮人工判断指定的轴。滑块通道（默认）直接给数值，不经过 LLM。
  std::optional<iteration_axes_t> feedback_axes_{};
  /// 候选的预设库，供检索起手
  std::vector<ai_light_preset> presets_{};
  /// 本轮渲染的**基线关卡**（UE UObject 路径，如 /Game/EP0001/Map/XXX.XXX），
  /// 会被复制进每个候选 rig 的 lighting_rig_t::baseline_level_。
  ///
  /// **本轮由调用方透传，生产者尚未接线**（§15.8 M2 阻塞项）：正确来源是
  /// auto_task::shot_render_light(...) 的 original_map_（含地编预调总关卡覆盖），
  /// 但那个函数同时做解算资产配对与文件校验，失败面很广。为空时接缝只记 WARN 不拦 ——
  /// UE worker（M2）落地前该字段没有消费者。
  std::string baseline_level_{};
  /// 人工提示词原文。只用于写入 sd2::task.text_prompt_（任务列表里可读）；
  /// 接缝正文用的是 spec_.intent_，不重复塞进 input_text。
  std::string prompt_text_{};
};

struct DOODLELIB_API candidate_generation_result_t {
  /// 必须恰好 g_candidate_count 个。第 0 个是本轮基线（variant_axis_.axis_ == none），
  /// 其余各自只沿一条轴变化（§15.8 OFAT 不变量）。
  std::vector<lighting_rig_t> rigs_{};
  iteration_axes_t axes_{};
};

/// Spec + 基线 rig + 上一轮反馈轴 → 本轮候选。
///
/// in_sql 用来解析 `operator_id_ → person.ai_studio_id_ → ai_studio`（取推理密钥）
/// 并交给 run_inference_task 落任务行与扣费。**不引入任何隐式全局**（决定 D1）。
///
/// 失败一律抛 doodle_error：内容违规 → bad_request；推理失败 / 输出不合解析 /
/// 基线缺主光 / 变异候选与基线相同 → bad_gateway；工作室或人员缺失 → bad_request。
/// 调用方 advance_session 负责回滚会话状态。**本函数不写任何 ai_light_* 表**。
DOODLELIB_API boost::asio::awaitable<candidate_generation_result_t> generate_candidates(
    sqlite_database& in_sql, const candidate_generation_request_t& in_request
);

// ---------------------------------------------------------------------------
// 接缝 2 的离线可测部件
// ---------------------------------------------------------------------------

// build_common_lighting_section() 定义在 ai_light_seam_common.h（三条接缝共用的库内部件）。
// 它不从伞形头导出：唯一的两个调用方（接缝 1 与接缝 2 的 build_*_instructions）都在库内，
// 而 seam_common 头要 include 推理客户端头 —— 把它挂进伞形头会把 HTTP 依赖带给每一个
// 包含本头的 TU。

/// 交给 LLM 的系统指令：共用段 + 轴表（§6.2）+ §6.6 护栏 + 轴值语义（§15.8 表）+ 输出要求。
DOODLELIB_API std::string build_candidates_instructions();

/// 结构化输出契约（方舟 Responses 的 text.format）。形状见 §15.8：
/// {strategy, axes[], globals{}, lights[]}，逐层 additionalProperties:false + 全字段必填。
DOODLELIB_API nlohmann::json build_candidates_json_schema();

/// 用户消息正文：Spec 紧凑投影 + 预设候选 + （若有）上一轮选中的 rig + （若有）反馈轴。
DOODLELIB_API std::string build_candidates_input_text(const candidate_generation_request_t& in_request);

/// 归一化推理信封。**不发 images**（Spec 已蒸馏了参考，生成阶段不需要图），
/// **不发 max_output_tokens**（缺省 4096 会截断，见 §15.6 坑 21），**不发 thinking**。
DOODLELIB_API http::seedance2::inference_request_t build_candidates_request(
    const candidate_generation_request_t& in_request
);

/// 没有可用轴计划时的确定性回退：两个轴、共 5 个变异位（1 基线 + 5 = g_candidate_count）。
/// 档位取自 §6.2 自己给的数字，不含中性值（中性档等于基线，会被 §9.1 的同一个候选检查拦下）。
DOODLELIB_API iteration_axes_t default_axis_plan(const lighting_spec_t& in_spec);

/// 模型给的精简形状 → 本轮基线 rig。服务端拥有 IR 细节（anchor/channels/mobility/origin/
/// id 生成），模型只给「灯怎么打」。枚举一律 magic_enum::enum_cast 校验，认不出抛 bad_gateway。
DOODLELIB_API lighting_rig_t build_base_rig(const nlohmann::json& in_json);

// apply_axis_step / expand_candidates / diff_rigs 与两个轴计划常量定义在 ai_light_rig_ops.h：
// 它们是纯 rig 运算，既不碰数据库也不碰接缝信封，放在接缝头里会让「接缝」这个词失去边界。

/// LLM 结果 → candidate_generation_result_t。服务端钉死 schema_version/身份字段/
/// baseline_level/provenance；轴计划不合法或档位总数不等于 g_candidate_count-1 时
/// **整体回退 default_axis_plan() 并把原因追加进 axes_.strategy_**（不抛错）。
/// in_result_json 非 object（含 null）抛 doodle_error{status::bad_gateway}。
DOODLELIB_API candidate_generation_result_t map_candidate_result(
    const nlohmann::json& in_result_json, const candidate_generation_request_t& in_request
);

}  // namespace doodle::ai_light
