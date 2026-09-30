//
// AI 灯光：轮次编排与状态机（docs/ue_ai_lighting_plan.md §10.3 / §13.1）
//
// 这一层刻意**不包含任何 HTTP 依赖**：
//   * 单测可以直接调状态机，不必拉起 session_data / JWT；
//   * 测试二进制不会被 http 头污染。
// 路由层（http_method/kitsu/ai_light.h）只是这一层的薄包装。
//
// ---------------------------------------------------------------------------
// 三个「接缝」：接入 LLM 的三个位置
// ---------------------------------------------------------------------------
// generate_spec 已落地（见 §15.7）：提示词 + 多图统计 → LightingSpec。
// generate_candidates 已落地（见 §15.8）：Spec + 基线 rig + 上一轮反馈轴 → 本轮 6 个候选。
// structure_feedback 已落地（见 §15.9）：自由文本 → 轴方向，填进
//   ai_light_judgement_t::feedback_axes_，下一轮当**约束**用。
// 三者都走已落地的方舟 Responses 通道（`seedance2_client` + `run_inference_task`，
// 用法见 §15.6）。**仓库里已经没有 not_implemented 的接缝了。**
//
// 接缝契约（实现方必须遵守）：
//   1. 抛出即视为「本轮无法推进」。调用方 advance_session() 会把会话状态回滚到进入时的
//      状态并记录 failure_reason_，所以实现方**不需要**自己清理状态。
//   2. 但必须保证「抛之前没有落库」，或者「落库的部分是幂等可重入的」——
//      调用方只回滚会话状态这一列，不会回滚接缝自己写下的东西。
//      **受控例外**：generate_spec / generate_candidates / structure_feedback 经由
//      run_inference_task 写下一行推理任务与对应的扣费记录。这是有意保留的 —— 推理确实
//      发生了，抹掉它等于抹掉计费凭证。除此之外它们不写任何 ai_light_* 表
//      （返回值全部交给调用方落库）。
//   3. 返回值必须自洽：候选数量 == g_candidate_count，且满足 §6.6 护栏
//      （同轮候选的灯集合与角色完全一致，只有轴参数不同）。调用方会再校验一次，
//      不合规直接判失败 —— 护栏是硬约束，不能靠实现方自觉。
//

#pragma once

#include <doodle_core/metadata/ai_light/preset.h>
#include <doodle_core/metadata/ai_light/view.h>
#include <doodle_core/metadata/preview_file.h>

#include <doodle_lib/doodle_lib_fwd.h>
// 归一化推理信封(强类型)。这个头只依赖 nlohmann/json + STL, 不带任何 HTTP 依赖,
// 所以不会破坏本文件顶部「这一层刻意不包含任何 HTTP 依赖」的约定。
#include <doodle_lib/http_client/seedance2_inference_request.h>

#include <boost/asio/awaitable.hpp>
#include <nlohmann/json.hpp>

#include <atomic>
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace doodle::ai_light {

// ===========================================================================
// 接缝 1：提示词（+ 参考图统计）→ LightingSpec
// ===========================================================================

struct DOODLELIB_API spec_generation_request_t {
  uuid session_id_{};
  /// 写入 sd2::task.project_uuid_id_（可空 FK）。只为归属可查，接缝内部不依赖它。
  uuid project_id_{};
  /// 触发人（person uuid）= 计费主体 + 签名 URL 里的 uid。
  ///
  /// 必须显式传入、**不给默认值**：`run_inference_task` 会按 `user_id_` 扣
  /// `person_token`，一旦有默认值，将来新增调用点忘记传就会把费用静默记到别人头上。
  /// 为 nil 时由 advance_session 回落到 `ai_light_session.submitter_`。
  uuid operator_id_{};
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
  /// 键为字段路径（如 "targets.contrast"），与 ai_light_spec::evidence_ 同形状 ——
  /// evidence 的语义就是「这个字段是从哪来的、有多可信」，没有键就失去意义。
  std::map<std::string, evidence_entry_t> evidence_{};
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
/// **本函数不写任何 ai_light_* 表**（受控例外见文件头的契约 2）。
DOODLELIB_API boost::asio::awaitable<spec_generation_result_t> generate_spec(
    sqlite_database& in_sql, const spec_generation_request_t& in_request
);

// ---------------------------------------------------------------------------
// 接缝 1 的离线可测部件
// ---------------------------------------------------------------------------
// generate_spec 被拆成「纯函数组信封/映射」+「一次真实网络调用」，前一半可以完全离线单测
// （单测宿主没有 SSL context / http logger，generate_spec 的成功路径没法在单测里跑）。
//
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
/// evidence、conflicts。枚举用 magic_enum::enum_cast 校验，认不出回退默认并记 evidence。
/// in_result_json 非 object（含 null）抛 doodle_error{status::bad_gateway}。
DOODLELIB_API spec_generation_result_t map_spec_result(
    const nlohmann::json& in_result_json, const aggregated_reference_stats_t& in_aggregated
);

// ===========================================================================
// 接缝 2：基线 rig + Spec → 本轮 6 个候选
// ===========================================================================

struct DOODLELIB_API candidate_generation_request_t {
  uuid session_id_{};
  /// 写入 sd2::task.project_uuid_id_（可空 FK）。只为归属可查，接缝内部不依赖它。
  uuid project_id_{};
  /// 触发人（person uuid）= 计费主体。语义与默认值规则同 spec_generation_request_t::operator_id_
  /// （**不给默认值**：有默认值则新调用点漏传会把扣费静默记到别人头上）。
  /// 为 nil 时由 advance_session 回落到 ai_light_session.submitter_；两者都 nil 抛 bad_request。
  uuid operator_id_{};
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
// 与接缝 1 同样的分法：纯函数组信封/映射 + 一次真实网络调用。约定也相同 ——
// 必须定义在命名空间 doodle::ai_light 内（匿名命名空间 = 内部链接 = 库外 LNK2019）。
//
// 与接缝 1 的关键差别：**展开器是确定性的**（§8.5 第 1 条）。模型只输出「一套基线灯
// 集合 + 一份轴计划」，6 个候选由 expand_candidates() 机械展开，所以
// 「同一基线 + 同一轴计划 → 同一组 rig」可以脱网复现与断言。

/// 抽取给 spec 与 candidates 共用的系统指令段：职责边界（§6.1）、性格 → 灯光语言映射表
/// （§5.5）、单位口径。**只允许有一处定义** —— 两份提示词对「对比度是什么」说法不一致
/// 会让模型在两条接缝上给出互不相容的数值。
DOODLELIB_API std::string build_common_lighting_section();

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

/// 把一条轴的一个档位应用到 rig 上，返回是否真的改动了（值为中性 / 缺主光 / 单位不可比 → false）。
/// 轴值语义见 §15.8 的表；**UE 侧必须按同一份口径实现**。
DOODLELIB_API bool apply_axis_step(lighting_rig_t& io_rig, const variant_axis_t& in_axis);

/// OFAT 展开：复制 in_base 作候选 0（variant_axis_ = none），再按 in_axes 的声明顺序逐个
/// 档位生成候选 1..N，每个候选只沿一条轴变化。返回大小 == 1 + Σ|steps|。
DOODLELIB_API std::vector<lighting_rig_t> expand_candidates(
    const lighting_rig_t& in_base, const iteration_axes_t& in_axes
);

/// 候选相对本轮基线的确定性 diff，写进 lighting_rig_t::ops_（候选 0 得空表）。
/// 供审计与前端展示，也让 UE 执行器能直接「按 op 精确增删」（§7.5，不用 ClearAllLight）。
DOODLELIB_API std::vector<light_op_t> diff_rigs(
    const lighting_rig_t& in_base, const lighting_rig_t& in_target
);

/// LLM 结果 → candidate_generation_result_t。服务端钉死 schema_version/身份字段/
/// baseline_level/provenance；轴计划不合法或档位总数不等于 g_candidate_count-1 时
/// **整体回退 default_axis_plan() 并把原因追加进 axes_.strategy_**（不抛错）。
/// in_result_json 非 object（含 null）抛 doodle_error{status::bad_gateway}。
DOODLELIB_API candidate_generation_result_t map_candidate_result(
    const nlohmann::json& in_result_json, const candidate_generation_request_t& in_request
);

// ===========================================================================
// 接缝 3：自由文本反馈 → 结构化轴调整
// ===========================================================================

struct DOODLELIB_API feedback_structuring_request_t {
  uuid session_id_{};
  /// 写入 sd2::task.project_uuid_id_（可空 FK）。只为归属可查，接缝内部不依赖它。
  uuid project_id_{};
  /// 触发人（person uuid）= 计费主体。语义与默认值规则同 spec_generation_request_t::operator_id_
  /// （**不给默认值**：有默认值则新调用点漏传会把扣费静默记到别人头上）。
  uuid operator_id_{};
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
/// 完整档位安排由下一轮 generate_candidates 的 axes_ 决定。空 vector 是合法结果
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
// 与前两条接缝同样的分法：纯函数组信封/映射 + 一次真实网络调用。

/// 系统指令：翻译环节的角色 + **共用**的轴值语义段（与 apply_axis_step 同一份口径）
/// + 只给方向 / 最多 2 条轴 / 别为凑数编一条 等约束。
DOODLELIB_API std::string build_feedback_instructions();

/// 结构化输出契约：{understanding, axes[{axis, value, reason}]}。
/// axis 的 enum 复用接缝 2 的 make_axis_enum_schema()（已剔除 none）；axes.maxItems == 2，
/// **刻意不设 minItems**（空数组是「美术没给出方向」的合法表达）。
DOODLELIB_API nlohmann::json build_feedback_json_schema();

/// 用户消息正文：美术原话 + 决定 + Spec 目标 + 本轮 6 个候选的对照结构 +
/// 选中候选的完整 rig + 当前 key:fill 比 + 滑块已给的方向。
DOODLELIB_API std::string build_feedback_input_text(const feedback_structuring_request_t& in_request);

/// 归一化推理信封。与另两条接缝一致：不发 images / image_detail / max_output_tokens / thinking。
DOODLELIB_API http::seedance2::inference_request_t build_feedback_request(
    const feedback_structuring_request_t& in_request
);

/// LLM 结果 → 轴方向。形状违规抛 bad_gateway；语义无效的轴丢弃并记 WARN（见结构体注释）。
DOODLELIB_API std::vector<axis_step_t> map_feedback_result(const nlohmann::json& in_result_json);

/// 当前 key:fill 比值（key 强度 / Σfill 强度）。条件与 apply_axis_step 的 key_fill_ratio
/// 分支完全一致：key 存在且 > 0、fill 与 key **同 intensity_unit_**（不同量纲没有比值）、
/// fill 和 > 0；否则 nullopt。
/// 提示词与执行器共用它 —— key_fill_ratio 是**绝对值**轴，模型必须知道现状才能给出新值。
DOODLELIB_API std::optional<double> key_fill_ratio_of(const lighting_rig_t& in_rig);

// ===========================================================================
// 状态机
// ===========================================================================

/// 会话状态推进的唯一入口。按 in_session.state_ 分派：
///
///   draft / awaiting_spec_confirm（前置：会话已有参考图）
///       → generate_spec 接缝 → 写 ai_light_spec(revision = max+1)
///       → session = awaiting_spec_confirm
///   generating 且已有已确认的 Spec
///       → generate_candidates 接缝 → 写 iteration + g_candidate_count 个 candidate
///       → 建 server_task_info{type = ai_light} → run_next_task() + broadcast
///       → session 保持 generating（渲染中）；渲染完由 mark_iteration_rendered 转 awaiting_judgement
///
/// 参考图为空时**拒绝**（400，且不写 failure_reason_）：Spec 的输入是提示词 + 参考图，
/// 少了参考图就只能蒸出一份 prompt-only 的方案，而那是主输入的静默丢失。
/// 因此建会话接口**不**调本函数 —— 建会话时参考图还不可能存在（见 shot_session_route::post）。
///
/// 失败时回滚会话状态到进入时的状态、记 failure_reason_，然后重新抛出。
/// 这样不会留下卡在 generating 的会话，也不会丢已上传的参考图
/// （参考图的外键 cascade 挂在会话上，删会话才会连带删图）。
///
/// 分派只看状态，所以**同一个状态只能有一个后继步骤** —— 这是结构性约束。
/// 「等判断」的会话没有可推进的步骤（该做的是 /judge，不是再推一步）。
///
/// in_operator_id 是本次推进的触发人（person uuid），转手给 generate_spec 做计费主体与
/// 签名 URL 的 uid。**不给默认值**：见 spec_generation_request_t::operator_id_ 的说明。
/// 为 nil 时回落到 in_session.submitter_；两者都 nil 抛 bad_request（无法解析工作室、
/// 无法计费）—— 这条在 try 内，所以照常回滚并记 failure_reason_。
DOODLELIB_API boost::asio::awaitable<void> advance_session(
    sqlite_database& in_sql, ai_light_session in_session, uuid in_operator_id
);

/// 人工闸门 2 的状态推进。**纯 DB，不碰文件与 LLM** —— 单测直接调它。
///
/// 前置条件（由调用方校验）：
///   * in_iteration 就是 in_judgement 所判断的那一轮；
///   * in_judgement.selected_candidate_id_ 属于 in_iteration（防跨轮误选）；
///   * decision == accept 时 in_accepted_preview_file 必须已填好（id / 尺寸 / 路径），
///     其余 decision 必须为空。
///
/// 为什么 preview_file 由调用方构造、却由本函数插入：定稿要同时写三处
/// （preview_file 行、candidate 的定稿标记与 preview_file_id_、会话状态），
/// 分两个事务提交就会出现「预览文件已建但候选没标定稿」这种半成品状态。
/// 调用方负责碰文件（OpenCV 读图 + save_variants 落变体），本函数负责一次事务落库。
///
/// preview_file 按值传入而非 const 引用：ORM 的 values() 会回填 uuid_id_ / created_at_
/// 这类服务端生成的字段，必须拿到可写对象。
///
/// 本函数负责：插入 preview_file、标记候选（accept → mark_accepted()；
/// continue → is_selected_ = true）、回填 candidate.preview_file_id_、
/// 写 iteration 的判断记录、推进 session 状态。
DOODLELIB_API boost::asio::awaitable<void> apply_judgement(
    sqlite_database& in_sql, ai_light_session& in_session, ai_light_iteration& in_iteration,
    const ai_light_judgement_t& in_judgement, std::optional<preview_file> in_accepted_preview_file = {}
);

/// worker 回传渲染完成后调用（§10.3 的「落 iteration → 置 awaiting_judgement」）。
///
/// 本轮没有 worker（§13.3 未做），所以这个函数只提供入口并纳入单测；
/// worker 里程碑把回传处理接上即可，状态机本身不用改。
/// in_candidates 是回传的候选（含 render_png_path_ / render_ms_ / light_count_ / metrics_）。
DOODLELIB_API boost::asio::awaitable<void> mark_iteration_rendered(
    sqlite_database& in_sql, const uuid& in_server_task_info_id, const std::vector<ai_light_candidate>& in_candidates
);

/// 组装会话详情读模型。会话不存在时返回 nullopt。
DOODLELIB_API std::optional<ai_light_session_detail_t> build_session_detail(
    sqlite_database& in_sql, const uuid& in_session_id
);

}  // namespace doodle::ai_light
