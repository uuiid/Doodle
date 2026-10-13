//
// AI 灯光：rig 纯运算（diff / key:fill 比）
//
// 这些函数是**纯函数**：只吃 lighting_rig_t，不碰数据库、不碰 HTTP、不碰日志。
// 单测（src/test/core/ai_light_feedback.cpp）直接调用它们。
//
// 独立成文件：状态机、接缝、rig 运算各占一个文件。本文件不依赖另外几个 ——
// 它是接缝 3 的**下游**工具，不是接缝本身。
//
// 曾经还有 apply_axis_step / expand_candidates（OFAT 展开器）与 g_axis_step_budget：
// 它们随候选生成一起归前端（角色降级 · 第二梯队），服务端不再展开候选 ——
// 候选由前端算好、经 POST /session/{}/iteration 提交，服务端只做 IR 准入闸门与落库。
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

/// 每轮最多声明几个轴。太多轴会让「差别从哪来」变得看不出来，而美术的判断正是收敛信号。
/// 接缝 3 的「最多 2 条轴」也引用它（决定 D12）。
inline constexpr std::size_t g_axis_max_count = 2;

/// 当前 key:fill 比值（key 强度 / Σfill 强度）。条件与轴值语义里 key_fill_ratio 的
/// 口径完全一致：key 存在且强度 > 0、所有 fill 灯与 key **同 intensity_unit_**
/// （坎德拉与勒克斯不是同一个量纲，比值没有意义）、fill 强度和 > 0；否则 nullopt。
///
/// 提示词与执行器共用它 —— key_fill_ratio 是**绝对值**轴，模型必须知道现状才能给出新值
/// （「对比再强一点」不是一个可直接使用的数字）。
DOODLELIB_API std::optional<double> key_fill_ratio_of(const lighting_rig_t& in_rig);

/// 候选相对本轮基线的确定性 diff，写进 lighting_rig_t::ops_（候选 0 得空表）。
/// 供审计与前端展示，也让 UE 执行器能直接「按 op 精确增删」（§7.5，不用 ClearAllLight）。
DOODLELIB_API std::vector<light_op_t> diff_rigs(
    const lighting_rig_t& in_base, const lighting_rig_t& in_target
);

}  // namespace doodle::ai_light
