//
// AI 灯光 · 接缝 1：提示词（+ 参考图统计）→ LightingSpec
// （设计见 docs/ue_ai_lighting_plan.md §15.7）
//
// 接缝分两层：纯函数组信封/映射 + 一次真实网络调用，前一层可以完全离线单测
// （单测宿主没有 SSL context / http logger，成功路径没法在单测里跑）。
//
// 伞形头 ai_light_orchestrator.h 会 include 本头 —— 对外仍然只需包含伞形头一个头。
//

#pragma once

#include <doodle_core/metadata/ai_light/reference.h>
#include <doodle_core/metadata/ai_light/spec.h>

#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_lib/http_client/seedance2_inference_request.h>

#include "ai_light_seam_common.h"

#include <boost/asio/awaitable.hpp>
#include <nlohmann/json.hpp>

#include <map>
#include <optional>
#include <string>
#include <vector>

namespace doodle::ai_light {

// ===========================================================================
// 接缝 1：提示词（+ 参考图统计）→ LightingSpec
// ===========================================================================

struct DOODLELIB_API spec_generation_request_t : public seam_request_base_t {
  /// 人工提示词原文（权威意图）。包含氛围、角色性格、故事上下文、背景设定。
  std::string prompt_text_{};
  /// 已上传的参考图。stats_ 已由统计层填好（§15.5），未分析过的图为 null（降级路径）。
  std::vector<ai_light_reference> references_{};
  /// 调用方按 references_have_stats() 给出的**预判**。接缝以它为初值，但若聚合器发现
  /// 根本没有可用统计就降级为 prompt_only —— 落库的 degrade_level_ 以事实为准（决定 D4）：
  /// 「有 stats 行但置信度全 0」「全部图被人工排除」这类情况下两者会分叉。
  spec_degrade_level degrade_level_{spec_degrade_level::prompt_only};
  /// 重出方案时的上一版（美术对上一版不满意 → 重新蒸馏）
  std::optional<lighting_spec_t> previous_spec_{};
};

struct DOODLELIB_API spec_generation_result_t {
  lighting_spec_t spec_{};
  reference_stats_t reference_stats_{};
  conflict_resolution_t conflict_resolution_{};
};

/// 提示词（+ 参考图统计）→ LightingSpec。
///
/// in_sql 用来解析 `operator_id_ → person.ai_studio_id_ → ai_studio`（取推理密钥）
/// 并交给 run_inference_task 落任务行与扣费。**不引入任何隐式全局**（决定 D1）。
///
/// 失败一律抛 doodle_error：内容违规 → bad_request；推理失败/输出不合解析 → bad_gateway；
/// 工作室或人员缺失 → bad_request。调用方 advance_session 负责回滚会话状态。
/// **本函数不写任何 ai_light_* 表**（受控例外见伞形头的契约 2）。
DOODLELIB_API boost::asio::awaitable<spec_generation_result_t> generate_spec(
    sqlite_database& in_sql, const spec_generation_request_t& in_request
);

// ---------------------------------------------------------------------------
// 接缝 1 的离线可测部件
// ---------------------------------------------------------------------------
// **这些定义必须在命名空间 doodle::ai_light 内，绝不能放进匿名命名空间** ——
// 匿名命名空间是内部链接：库内全绿，库外引用直接 LNK2019（§15.6 第 9 条踩过）。
//
// 参数用 `const aggregated_reference_stats_t&`（只前向声明）而不是把
// ai_light_stats.h 拖进来：那会把 opencv 头带进每一个包含本头文件的 TU。
// 调用方与单测按需自己包含 ai_light_stats.h。

struct aggregated_reference_stats_t;

/// 交给 LLM 的系统指令：职责边界（§6.1）、冲突规则（§6.3，**提示词优先于参考**）、
/// 性格 → 灯光语言映射表（§5.5）、单位口径、禁止事项（不得改几何/材质）。
DOODLELIB_API std::string build_spec_instructions();

/// 结构化输出契约（方舟 Responses 的 text.format）。平铺形状，见 §15.6 第 4 条。
DOODLELIB_API nlohmann::json build_spec_json_schema();

/// 用户消息正文：提示词原文 + 逐图统计摘要 + 聚合目标 + 上一版投影。
DOODLELIB_API std::string build_spec_input_text(
    const spec_generation_request_t& in_request, const aggregated_reference_stats_t& in_aggregated
);

/// 归一化推理信封（build_inference_body 的输入）。**不发** images 为空数组 —— 由
/// inference_request_t 的 to_json 保证：空 vector 不写 images 键。
DOODLELIB_API http::seedance2::inference_request_t build_spec_request(
    const spec_generation_request_t& in_request, const aggregated_reference_stats_t& in_aggregated
);

/// LLM 结果 → spec_generation_result_t。服务端钉死 space/palette/ownership/schema_version/
/// reference_stats/身份字段；LLM 只能给 intent、targets 的标量区间与 grading_hints、
/// conflicts。自由文本枚举走白名单校验，认不出就保留统计推导的值。
/// in_result_json 非 object（含 null）抛 doodle_error{status::bad_gateway}。
DOODLELIB_API spec_generation_result_t map_spec_result(
    const nlohmann::json& in_result_json, const aggregated_reference_stats_t& in_aggregated
);

}  // namespace doodle::ai_light
