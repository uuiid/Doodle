//
// AI 灯光：rig 纯运算（轴施加 / OFAT 展开 / diff / key:fill 比）
//
// 这些函数是**纯函数**：只吃 lighting_rig_t 与轴，不碰数据库、不碰 HTTP、不碰日志。
// 单测（src/test/core/ai_light_candidates.cpp）直接调用它们；UE 执行器也必须按同一份
// 口径实现轴值语义 —— 「模型按它给档位，服务端按同一套公式应用」，两边不一致时
// 美术会看到「轴变了但图没变」（§15.8 / §15.11）。
//
// 从 ai_light_orchestrator.h 拆出（重构方案 §2/§4）：状态机、三条接缝、rig 运算各自
// 一个文件。本文件不依赖另外三个 —— 它是接缝 2 与接缝 3 的**下游**工具，不是接缝本身。
//

#pragma once

#include <doodle_core/metadata/ai_light/common.h>
#include <doodle_core/metadata/ai_light/iteration.h>
#include <doodle_core/metadata/ai_light/rig.h>

#include <doodle_lib/doodle_lib_fwd.h>

#include <cstddef>
#include <optional>
#include <vector>

namespace doodle::ai_light {

/// 轴计划允许的变异位总数：1 个基线 + 5 个变异位 == g_candidate_count。
///
/// 与 g_candidate_count 一样是**产品常量**：expand_candidates 的 reserve、
/// parse_axis_plan / build_candidates_json_schema 的档位上限共用它，
/// 所以它必须是唯一的一份定义（原先在 ai_light_orchestrator.cpp 的匿名命名空间里，
/// 拆文件后放这里，见 §15.11）。
inline constexpr std::size_t g_axis_step_budget = static_cast<std::size_t>(g_candidate_count) - 1;

/// 每轮最多声明几个轴。太多轴会让「差别从哪来」变得看不出来，而美术的判断正是收敛信号。
/// 接缝 3 的「最多 2 条轴」也引用它（决定 D12）。
inline constexpr std::size_t g_axis_max_count = 2;

/// 当前 key:fill 比值（key 强度 / Σfill 强度）。条件与 apply_axis_step 的 key_fill_ratio
/// 分支完全一致：key 存在且强度 > 0、所有 fill 灯与 key **同 intensity_unit_**
/// （坎德拉与勒克斯不是同一个量纲，比值没有意义）、fill 强度和 > 0；否则 nullopt。
///
/// 提示词与执行器共用它 —— key_fill_ratio 是**绝对值**轴，模型必须知道现状才能给出新值
/// （「对比再强一点」不是一个可直接使用的数字）。
DOODLELIB_API std::optional<double> key_fill_ratio_of(const lighting_rig_t& in_rig);

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

}  // namespace doodle::ai_light
