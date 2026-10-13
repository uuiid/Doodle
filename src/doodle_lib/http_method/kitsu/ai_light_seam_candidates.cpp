//
// AI 灯光 · 接缝 2 实现：基线 rig + Spec → 本轮 6 个候选
// （设计见 docs/ue_ai_lighting_plan.md §15.8）
//
// 离线可测部件（build_candidates_* / default_axis_plan / build_base_rig / parse_axis_plan /
// map_candidate_result 及其文件内助手）与联网入口 generate_candidates 都定义在这里；
// 声明在 ai_light_seam_candidates.h。
//

#include "ai_light_seam_candidates.h"

#include "ai_light_json_validation.h"
#include "ai_light_rig_ops.h"
#include "ai_light_seam_common.h"

#include <doodle_core/exception/exception.h>
#include <doodle_core/metadata/ai_light/ai_light.h>
#include <doodle_core/metadata/entity.h>
#include <doodle_core/metadata/kitsu_ctx_t.h>
#include <doodle_core/metadata/person.h>
#include <doodle_core/metadata/server_task_info.h>
#include <doodle_core/configure/static_value.h>

#include <doodle_lib/core/core_set.h>
#include <doodle_lib/core/global_function.h>
#include <doodle_lib/core/socket_io/broadcast.h>
#include <doodle_lib/http_client/seedance2_client.h>
#include <doodle_lib/http_method/kitsu/ai_light.h>
#include <doodle_lib/http_method/kitsu/ai_light_stats.h>
#include <doodle_lib/http_method/kitsu/computers.h>
#include <doodle_lib/http_method/seedance2/inference_task.h>
#include <doodle_lib/logger/logger.h>
#include <doodle_lib/sqlite_orm/orm/orm.h>
#include <doodle_lib/sqlite_orm/sqlite_database.h>

#include <magic_enum/magic_enum.hpp>
#include <nlohmann/json.hpp>

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <exception>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace doodle::ai_light {

// ORM 的构造助手都在 doodle::orm 里（本文件的映射暂不碰库，但保持与另两条接缝一致）。
using namespace orm;

// ===========================================================================
// 接缝 2：Spec + 基线 rig + 上一轮反馈轴 → 本轮 6 个候选（方案 §6.2 / §6.6 / §9.1）
// ===========================================================================
//
// 分层与接缝 1 相同（纯函数组信封/映射 + 一次网络调用），但展开器是**确定性**的：
// 模型只输出「一套基线灯集合 + 一份轴计划」，6 个候选由 expand_candidates() 机械展开。
// 于是「同一基线 + 同一轴计划 → 同一组 rig」可以脱网复现（§8.5 第 1 条），
// §6.6 的「同轮候选灯集合与角色完全一致」由构造直接保证，不需要事后校验。
//
// 为什么不给每个候选一次推理：§6.6 要求 6 个候选的灯集合与角色完全相同，只有轴参数不同。
// 让模型分别生成 6 套 rig，就必须事后校验「灯集合是否真的一致」，而模型一旦漂移，本轮
// 要么整体作废要么放弃护栏。分开「决策」（模型：往哪个方向搜）与「展开」（服务端：这 6 张
// 图分别是什么）之后，护栏变成构造性事实。

namespace {

/// axes_.strategy_ 是 TEXT 列，模型可以长篇大论；截断避免一次输出把这一列撑爆。
constexpr std::size_t g_strategy_max_len = 500;

/// 提示词里最多列几个预设。预设库首批只有几十条，全在提示词里列一遍没有必要。
constexpr std::size_t g_preset_prompt_top_k = 12;


// ---- 严格取值器 ----------------------------------------------------------
// 基线 rig 的物化是「模型给什么就用什么」的那一段，所以这里**不用**宽松的 opt_xxx
// （那套是「能读就读、读不出用默认」）。strict schema 下缺字段本该不可能发生；发生了说明
// 模型没遵守契约，静默取默认值会把「模型没按要求输出」变成一个看不出来的偏移。
//
// 实现在 ai_light_json_validation.h 的 json_val::require_*：接缝名与字段路径由调用方传入，
// 枚举一律走 magic_enum::enum_cast 显式校验。这样错误消息里带的是**调用方**的接缝名，
// 同一个读取器三条接缝都能用。

// ---- rig 取值助手 --------------------------------------------------------

/// 只比较「灯怎么打」的部分：身份字段（rig_id/spec_id/iteration/candidate_index）、
/// variant_axis_、provenance_、ops_ 都由服务端另填，拿它们判「候选是否与基线相同」会误判。
nlohmann::json rig_substance(const lighting_rig_t& in_rig) {
  return nlohmann::json{{"globals", in_rig.globals_}, {"lights", in_rig.lights_}};
}



/// 预设检索走词法，不走向量：全仓库**没有**可用于「按 Spec 检索预设」的查询向量生成器
/// （唯一的 embedding 生成器是 ONNX 的 ai/llm2vec，被 kimodo 用，与预设的 CLIP 图编码
/// 不是一回事）。中文没有词边界，所以用子串包含而不是分词 ——「霓虹」⊂「夜景霓虹」正是要的。
std::size_t preset_lexical_score(const ai_light_preset& in_preset, const std::string& in_haystack) {
  std::size_t l_score = 0;
  if (!in_preset.name_.empty() && in_haystack.find(in_preset.name_) != std::string::npos) l_score += 4;
  for (const auto& l_tag : in_preset.tags_)
    if (!l_tag.empty() && in_haystack.find(l_tag) != std::string::npos) l_score += 2;
  return l_score;
}

/// 同分按名字排序，保证同一份 Spec 得到同一份提示词（§8.5 第 1 条：可复现）。
std::vector<const ai_light_preset*> pick_presets(
    const candidate_generation_request_t& in_request, std::size_t in_top_k
) {
  std::string l_haystack = in_request.spec_.intent_.time_of_day_ + in_request.spec_.intent_.weather_ +
                           in_request.spec_.intent_.setting_ + in_request.spec_.intent_.story_context_;
  for (const auto& l_v : in_request.spec_.intent_.atmosphere_) l_haystack += l_v;
  for (const auto& l_v : in_request.spec_.intent_.source_motivation_) l_haystack += l_v;
  for (const auto& l_v : in_request.spec_.intent_.character_personality_) l_haystack += l_v.trait_ + l_v.subject_;

  std::vector<const ai_light_preset*> l_cands{};
  l_cands.reserve(in_request.presets_.size());
  for (const auto& l_preset : in_request.presets_)
    if (l_preset.enabled_) l_cands.emplace_back(&l_preset);

  std::sort(l_cands.begin(), l_cands.end(), [&l_haystack](const ai_light_preset* in_l, const ai_light_preset* in_r) {
    const auto l_sl = preset_lexical_score(*in_l, l_haystack);
    const auto l_sr = preset_lexical_score(*in_r, l_haystack);
    if (l_sl != l_sr) return l_sl > l_sr;
    return in_l->name_ < in_r->name_;
  });
  if (l_cands.size() > in_top_k) l_cands.resize(in_top_k);
  return l_cands;
}


/// 解析并校验模型的轴计划。返回失败原因（nullopt = 合法）。
///
/// 校验项都针对「静默失败会变成 6 张看不出区别的图」这一类：轴名是否合法、是否重复、
/// 档位总数够不够、有没有中性档。**不合法不抛错** —— 轴计划是模型的一种建议，弃用它
/// 退回默认计划仍然能给出 6 个可用候选，比让整轮失败好。
std::optional<std::string> parse_axis_plan(
    const nlohmann::json& in_json, iteration_axes_t& out_axes, std::string& out_strategy
) {
  out_strategy = json_val::opt_string(in_json, "strategy").value_or(std::string{});
  if (out_strategy.size() > g_strategy_max_len) out_strategy.resize(g_strategy_max_len);

  if (!in_json.is_object() || !in_json.contains("axes") || !in_json.at("axes").is_array())
    return std::string{"模型没有给出 axes 数组"};
  const auto& l_axes_json = in_json.at("axes");
  if (l_axes_json.empty() || l_axes_json.size() > g_axis_max_count)
    return fmt::format("axes 有 {} 个（只允许 1..{} 个）", l_axes_json.size(), g_axis_max_count);

  std::vector<axis_step_t> l_steps{};
  std::size_t l_total = 0;
  for (const auto& l_item : l_axes_json) {
    if (!l_item.is_object() || !l_item.contains("axis") || !l_item.at("axis").is_string())
      return std::string{"axes 的元素缺少 axis"};
    const auto l_kind = magic_enum::enum_cast<variant_axis_kind>(l_item.at("axis").get<std::string>());
    if (!l_kind.has_value()) return fmt::format("轴名 \"{}\" 不是合法的轴", l_item.at("axis").get<std::string>());
    if (*l_kind == variant_axis_kind::none) return std::string{"none 不是可搜索的轴"};
    for (const auto& l_prev : l_steps)
      if (l_prev.axis_ == *l_kind) return fmt::format("轴 {} 出现了两次", magic_enum::enum_name(*l_kind));

    if (!l_item.contains("steps") || !l_item.at("steps").is_array() || l_item.at("steps").empty())
      return fmt::format("轴 {} 没有 steps", magic_enum::enum_name(*l_kind));

    axis_step_t l_step{};
    l_step.axis_  = *l_kind;
    l_step.label_ = json_val::opt_string(l_item, "label").value_or(std::string{});
    if (l_step.label_.empty()) l_step.label_ = std::string{magic_enum::enum_name(*l_kind)};
    for (const auto& l_v : l_item.at("steps")) {
      if (!l_v.is_number()) return fmt::format("轴 {} 的档位里有一个不是数字", magic_enum::enum_name(*l_kind));
      const double l_value = l_v.get<double>();
      // key_direction / kelvin / hardness / ambient_level / grading_strength 的中性值都是 0；
      // key_fill_ratio 是比值，本身不存在 0 档。放行 0 档会当场产生一个与候选 0 相同的候选，
      // 服务端的同一个候选检查会在后面判失败 —— 不如在这里给出可读的原因。
      if (l_value == 0.0)
        return fmt::format("轴 {} 的档位包含 0（等价于基线，会得到一个重复候选）", magic_enum::enum_name(*l_kind));
      l_step.steps_.emplace_back(l_value);
    }
    l_total += l_step.steps_.size();
    l_steps.emplace_back(std::move(l_step));
  }
  if (l_total != g_axis_step_budget)
    return fmt::format(
        "档位总数是 {}，必须是 {}（1 个基线 + {} 个变异 = {} 个候选）", l_total, g_axis_step_budget,
        g_axis_step_budget, g_candidate_count
    );

  out_axes.axes_ = std::move(l_steps);
  return std::nullopt;
}

}  // namespace

std::string build_candidates_instructions() {
  std::string l_out{"你是电影灯光指导的**执行**环节。已经有一份人工确认过的灯光方案（LightingSpec），"
                    "现在要把它变成一组可以真的渲出来的灯光设置，并给出下一轮要试的方向。\n\n"};
  l_out += build_common_lighting_section();
  l_out += "\n【你要输出两样东西】\n"
           "1) 一套**基线灯集合**（globals + lights）：这是本轮 6 个候选共同的起点。\n"
           "2) 一份**轴计划**（axes）：在基线上沿哪几个方向、各试哪几档，共 5 个档位。\n"
           "6 个候选 = 1 个基线 + 5 个变异，由服务端按你的轴计划机械展开 —— 你**只给一套灯**，\n"
           "不要输出 6 套 rig。这样 6 张图的灯集合与角色必然一致（方案 §6.6 的护栏）。\n\n"
           "【轴值语义】\n";
  l_out += g_axis_value_semantics_text;
  // 排轴环节专属的硬约束。放在这里而不是语义段里：接缝 3 复用同一份语义段，
  // 但它的产出是「每条轴一个方向值」，套用这套档位规则会自相矛盾（§15.9 决定）。
  l_out += "\n硬约束：所有轴加起来必须有恰好 5 个档位（1 个基线 + 5 个变异 = 6 个候选）。\n"
           "档位不要取 0：0 档等价于基线，会得到一个与基线一模一样的候选，服务端会因此判本轮失败。\n"
           "最多声明 2 个轴（每轮只动少数几个自由度，美术才看得出差别从哪来）。\n";
  l_out += "\n【第一轮与后续轮次】\n"
           "首轮（没有上一轮 rig）：从 Spec 与预设出发搭一套完整灯集合，档位取**大跨度**，\n"
           "目的是先把搜索空间铺开，让美术一眼看出各方向差在哪。\n"
           "后续轮：以上一轮美术选中的 rig 为起点，**只在需要改的地方改**，档位取**小跨度**，\n"
           "在美术选中的区域附近收缩细化（§9.1：把人工判断从 yes/no 变成有效的搜索梯度）。\n\n"
           "【约束】\n"
           "- 至少要有 role=key 的主光：key_direction 与 key_fill_ratio 两个轴都作用在它身上，\n"
           "  没有主光这两个轴就是空操作。\n"
           "- 灯的角色与数量由你定，但同一轮内 6 个候选共用同一套灯（服务端保证）。\n"
           "- 曝光在迭代期必须手动锁定（method 固定 manual、ev100_bias 人为给定），\n"
           "  否则自动曝光会把强度改动吃掉，美术看到的差别来自测光而不是布光。\n"
           "- 环境光也算灯：天光强度、雾密度在 globals 里给。\n\n"
           "【输出要求】\n"
           "只输出符合给定 JSON Schema 的 JSON。不要输出解释文字，不要用 Markdown 代码块。\n"
           "color_mode 决定 color 的哪个分支生效：temperature 时填 kelvin，rgb 时填 rgb 三元组，\n"
           "filter 时填 filter 名字；另外两个分支按 schema 给中性占位（rgb 填 [0,0,0]、filter 填空串）。\n";
  return l_out;
}

nlohmann::json build_candidates_json_schema() {
  const auto l_axis_item = json_val::schema_object({
      {"axis", json_val::schema_enum<variant_axis_kind>(variant_axis_kind::none)},
      {"steps", json_val::schema_array(json_val::schema_number(), std::size_t{1}, g_axis_step_budget)},
      {"label", json_val::schema_string()},
  });

  const auto l_globals = json_val::schema_object({
      {"exposure_ev100_bias", json_val::schema_number()},
      {"white_balance_kelvin", json_val::schema_number(1500.0, 15000.0)},
      {"sky_light_intensity", json_val::schema_number(0.0)},
      {"fog_density", json_val::schema_number(0.0)},
      {"grading_contrast", json_val::schema_number(0.0, 3.0)},
      {"grading_saturation", json_val::schema_number(0.0, 3.0)},
      {"grading_lift", json_val::schema_number(0.0, 1.0)},
      {"grading_vignette", json_val::schema_number(0.0, 1.0)},
      {"grading_bloom", json_val::schema_number(0.0, 1.0)},
  });

  const auto l_light = json_val::schema_object({
      {"role", json_val::schema_enum<light_role>()},
      {"type", json_val::schema_enum<light_type>()},
      {"azimuth_deg", json_val::schema_number(-180.0, 180.0)},
      {"elevation_deg", json_val::schema_number(-90.0, 90.0)},
      {"distance_m", json_val::schema_number(0.1)},
      {"intensity_unit", json_val::schema_enum<intensity_unit>()},
      {"intensity", json_val::schema_number(0.0)},
      {"color_mode", json_val::schema_enum<color_mode>()},
      {"kelvin", json_val::schema_number(1500.0, 15000.0)},
      {"rgb", json_val::schema_array(json_val::schema_number(0.0, 1.0), std::size_t{3}, std::size_t{3})},
      {"filter", json_val::schema_string()},
      {"softness", json_val::schema_number(0.0, 1.0)},
      {"source_radius_cm", json_val::schema_number(0.0)},
      {"cast_shadows", json_val::schema_bool()},
      {"note", json_val::schema_string()},
  });

  // 全字段必填：颜色三个分支只能有一个生效，但严格结构化输出不支持「条件必填」，
  // 所以三个分支的字段都必填，非本分支的填中性占位（提示词里已说明）。
  //
  // 刻意没有：variant_axis / baseline_level / ops / rig_id / candidate_index / schema_version ——
  // 这些是服务端的字段（谁在哪一轮、哪个候选、从哪个关卡起手），模型给它们没有意义，
  // 而一旦进了 schema，模型就会开始编造它们。
  return json_val::schema_object({
      {"strategy", json_val::schema_string()},
      {"axes", json_val::schema_array(l_axis_item, std::size_t{1}, g_axis_max_count)},
      {"globals", l_globals},
      // 不设 maxItems：决策 17 明确「单镜头动态阴影灯上限先不限制」，这里加一个上限
      // 等于在 schema 层偷偷引入一条新的性能约束。
      {"lights", json_val::schema_array(l_light, std::size_t{1})},
  });
}

std::string build_candidates_input_text(const candidate_generation_request_t& in_request) {
  std::string l_out{};
  l_out += fmt::format(
      "【已确认的灯光方案（LightingSpec，人工已确认）】降级级别={}\n",
      magic_enum::enum_name(in_request.spec_.degrade_level_)
  );
  l_out += describe_spec_brief(in_request.spec_);

  const auto l_presets = pick_presets(in_request, g_preset_prompt_top_k);
  if (l_presets.empty()) {
    l_out += "\n【预设库】空的（或都被停用了）：请完全依据上面的方案自己给出一套基线灯集合。\n";
  } else {
    l_out += fmt::format("\n【可选预设（按标签匹配挑出的 {} 条，可以只借鉴其中一部分）】\n", l_presets.size());
    for (const auto* l_preset : l_presets) l_out += describe_preset_brief(*l_preset);
  }

  if (in_request.base_rig_.has_value()) {
    l_out += fmt::format(
        "\n【上一轮美术选中的 rig（第 {} 轮的起点，warm start）】\n", in_request.iteration_ - 1
    );
    l_out += describe_rig_brief(*in_request.base_rig_);
    l_out += "除非下面的 Spec 或反馈明确要求，**保持灯的角色与数量不变**，只调整数值与方位。\n";
  } else {
    l_out += "\n【首轮：没有上一轮的 rig】\n"
             "请从上面的方案与预设出发给出一套完整的基线灯集合；首轮档位取大跨度。\n";
  }

  if (in_request.feedback_axes_.has_value() && !in_request.feedback_axes_->axes_.empty()) {
    l_out += "\n【美术用滑块指定的方向（这些轴必须出现在你的 axes 里）】\n";
    for (const auto& l_axis : in_request.feedback_axes_->axes_)
      l_out += fmt::format("  {} = {}\n", magic_enum::enum_name(l_axis.axis_), join_numbers(l_axis.steps_));
    l_out += "美术已经把方向指出来了：请让对应轴的档位覆盖（或落在）这个方向附近，"
             "并把档位总数补齐到 5。\n";
  } else if (in_request.iteration_ > 1) {
    l_out += "\n【本轮没有滑块指定的方向】\n"
             "美术只是要求继续迭代；请根据上一轮选中的 rig 与目标之间的差距自己选轴，档位取小跨度。\n";
  }
  return l_out;
}

http::seedance2::inference_request_t build_candidates_request(const candidate_generation_request_t& in_request) {
  http::seedance2::inference_request_t l_req{};
  l_req.instructions_       = build_candidates_instructions();
  l_req.input_text_         = build_candidates_input_text(in_request);
  l_req.temperature_        = 0.2;
  l_req.json_schema_        = build_candidates_json_schema();
  l_req.json_schema_name_   = "lighting_candidates";
  l_req.json_schema_strict_ = true;
  // 刻意**不发**的键，每一个都有理由：
  //   images / image_detail  —— Spec 已经把参考蒸馏成数字目标了，生成阶段不需要再看图；
  //                             没有 images 还发 image_detail 就是给「不存在的输入」配参数。
  //   max_output_tokens      —— 缺省值 4096 会截断较大的输出，而截断的表现（result_json_ 变
  //                             null）与「schema 被拒」无法区分，排查时会走错方向（§15.6 坑 21）。
  //   thinking               —— 缺省 disabled；结构化任务开 thinking 会让 output[0] 变成
  //                             reasoning（§15.7）。
  return l_req;
}

iteration_axes_t default_axis_plan(const lighting_spec_t& in_spec) {
  // 档位直接取自 §6.2 自己给出的数字（3:1 / 6:1 / 10:1 与冷暖），是「模型没能给出可用搜索
  // 方向」时唯一有依据的起点。刻意不含中性档：中性档等于基线，会被同一个候选检查拦下。
  iteration_axes_t l_axes{};
  axis_step_t l_ratio{};
  l_ratio.axis_  = variant_axis_kind::key_fill_ratio;
  l_ratio.steps_ = {3.0, 6.0, 10.0};
  l_ratio.label_ = "对比档位";
  axis_step_t l_kelvin{};
  l_kelvin.axis_  = variant_axis_kind::kelvin;
  l_kelvin.steps_ = {-800.0, 800.0};
  l_kelvin.label_ = "冷暖";
  l_axes.axes_    = {l_ratio, l_kelvin};
  l_axes.strategy_ = fmt::format(
      "默认轴计划：先用 key:fill 的 {} 三档看对比结构，再用色温 {} 两档看冷暖。", join_numbers(l_ratio.steps_),
      join_numbers(l_kelvin.steps_)
  );
  if (!in_spec.reference_stats_.conflicts_.empty())
    l_axes.strategy_ += fmt::format(
        "（参考图存在冲突维度，未设为硬目标：{}）", join_strings(in_spec.reference_stats_.conflicts_, ", ")
    );
  return l_axes;
}

lighting_rig_t build_base_rig(const nlohmann::json& in_json) {
  // 接缝名进错误消息（F1）。这条接缝是**严格**的：读不出来就是 502，所以消息必须能直接
  // 指出「哪条接缝、哪个字段」—— 重构前写死的措辞只有「候选基线 rig 的字段 {}」。
  constexpr std::string_view l_seam{"候选基线 rig"};

  DOODLE_CHICK_HTTP(in_json.is_object(), bad_gateway, "候选基线 rig 的输出不是 JSON 对象");
  DOODLE_CHICK_HTTP(
      in_json.contains("globals") && in_json.at("globals").is_object(), bad_gateway, "候选基线 rig 缺少 globals 对象"
  );
  DOODLE_CHICK_HTTP(
      in_json.contains("lights") && in_json.at("lights").is_array() && !in_json.at("lights").empty(), bad_gateway,
      "候选基线 rig 缺少 lights 数组（至少要有一盏灯）"
  );
  const auto& l_lights = in_json.at("lights");

  lighting_rig_t l_rig{};
  l_rig.schema_version_ = std::string{g_rig_schema_version};

  // 路径一律相对**整个输出**（而不是相对 globals 子对象）传进去：json_at_path 拿同一个
  // 字符串取值，所以消息里印出来的 globals. 前缀就是真正被读的那条路径，不会错位。
  l_rig.globals_.exposure_.ev100_bias_ = json_val::require_number(in_json, l_seam, "globals.exposure_ev100_bias");
  l_rig.globals_.exposure_.method_     = "manual";  // §7.5：迭代期曝光必须手动锁定
  l_rig.globals_.white_balance_.kelvin_  = json_val::require_number(in_json, l_seam, "globals.white_balance_kelvin");
  l_rig.globals_.white_balance_.tint_    = 0.0;  // 色偏暂不进 schema（见 §15.8 待实测）
  l_rig.globals_.environment_.sky_light_intensity_ =
      json_val::require_number(in_json, l_seam, "globals.sky_light_intensity");
  l_rig.globals_.environment_.fog_density_ = json_val::require_number(in_json, l_seam, "globals.fog_density");
  l_rig.globals_.grading_.contrast_        = json_val::require_number(in_json, l_seam, "globals.grading_contrast");
  l_rig.globals_.grading_.saturation_      = json_val::require_number(in_json, l_seam, "globals.grading_saturation");
  l_rig.globals_.grading_.lift_            = json_val::require_number(in_json, l_seam, "globals.grading_lift");
  l_rig.globals_.grading_.vignette_        = json_val::require_number(in_json, l_seam, "globals.grading_vignette");
  l_rig.globals_.grading_.bloom_           = json_val::require_number(in_json, l_seam, "globals.grading_bloom");

  std::int32_t l_index = 0;
  for (const auto& l_json : l_lights) {
    // 灯的字段路径带下标：一批 6 个候选里「第 3 盏灯没给 softness」才定位得出来。
    const auto l_at = fmt::format("lights[{}].", l_index);
    DOODLE_CHICK_HTTP(l_json.is_object(), bad_gateway, "候选基线 rig 的 lights[{}] 不是对象", l_index);
    light_t l_light{};
    // id 由服务端生成：模型给的 id 可能与它自己的其它灯重名，而 id 是 ops_ 的对齐键，
    // 重名会让 diff 与执行器都指向错误的灯。
    l_light.id_                      = fmt::format("L{:02}", l_index++);
    l_light.role_                    = json_val::require_enum<light_role>(in_json, l_seam, l_at + "role");
    l_light.type_                    = json_val::require_enum<light_type>(in_json, l_seam, l_at + "type");
    l_light.offset_.azimuth_deg_     = json_val::require_number(in_json, l_seam, l_at + "azimuth_deg");
    l_light.offset_.elevation_deg_   = json_val::require_number(in_json, l_seam, l_at + "elevation_deg");
    l_light.offset_.distance_m_      = json_val::require_number(in_json, l_seam, l_at + "distance_m");
    l_light.intensity_.unit_ = json_val::require_enum<intensity_unit>(in_json, l_seam, l_at + "intensity_unit");
    l_light.intensity_.value_        = json_val::require_number(in_json, l_seam, l_at + "intensity");
    l_light.shadows_.softness_ = std::clamp(json_val::require_number(in_json, l_seam, l_at + "softness"), 0.0, 1.0);
    l_light.shadows_.cast_     = json_val::require_bool(in_json, l_seam, l_at + "cast_shadows");
    l_light.shape_.source_radius_cm_ =
        std::max(0.0, json_val::require_number(in_json, l_seam, l_at + "source_radius_cm"));
    l_light.color_.mode_ = json_val::require_enum<color_mode>(in_json, l_seam, l_at + "color_mode");
    // rgb 在 schema 里是必填三元组，**形状与 color_mode 无关地校验**：长度不对就说明模型没有按
    // schema 回答。若因为「当前模式用不到它」而静默忽略，一次明确的协议违规就变成了一盏看起来
    // 完全正常的灯 —— 与裸 `get<Enum>()` 静默落值属于同一类问题。
    if (const auto l_rgb_it = l_json.find("rgb"); l_rgb_it != l_json.end() && !l_rgb_it->is_null())
      (void)json_val::require_vec3(in_json, l_seam, l_at + "rgb");
    // 颜色三个分支**按 color_mode 路由，各自严格读**（F2/F3）：答了 color_mode=rgb 却没给
    // rgb（或缺 kelvin / filter）在重构前会被静默吞掉，得到一盏看起来正常的默认色灯。
    // 非当前分支的字段缺失/null 继续容忍 —— schema 里三分支都必填，但提示词明确说了
    // 非本分支填中性占位，所以这里不能要求它们。
    switch (l_light.color_.mode_) {
      case color_mode::temperature:
        l_light.color_.kelvin_ = json_val::require_number(in_json, l_seam, l_at + "kelvin");
        break;
      case color_mode::rgb: {
        const auto l_rgb    = json_val::require_vec3(in_json, l_seam, l_at + "rgb");
        l_light.color_.rgb_ = vec3_t{l_rgb.at(0), l_rgb.at(1), l_rgb.at(2)};
        break;
      }
      case color_mode::filter:
        l_light.color_.filter_ = json_val::require_string(in_json, l_seam, l_at + "filter");
        break;
      default:
        break;
    }
    // 其余 IR 字段走结构体默认值（anchor 默认相机空间、channels 默认 [0,1,0] 主光走 Channel1、
    // mobility=movable、origin=ai、cone 角留给执行器按灯型补）—— 这些是 IR 细节，
    // 让模型填只会多一处可以填错的地方。
    l_rig.lights_.emplace_back(std::move(l_light));
  }
  return l_rig;
}

candidate_generation_result_t map_candidate_result(
    const nlohmann::json& in_result_json, const candidate_generation_request_t& in_request
) {
  DOODLE_CHICK_HTTP(
      in_result_json.is_object(), bad_gateway, "候选生成接缝收到了非对象的推理结果（{}），无法映射为候选",
      in_result_json.type_name()
  );

  // ---- 1) 物化基线 rig（模型给「灯怎么打」，服务端补 IR 细节）----
  auto l_base = build_base_rig(in_result_json);

  // ---- 2) 服务端钉死：身份与来源字段一律不取模型的值 ----
  l_base.session_id_       = in_request.session_id_;
  l_base.spec_id_          = in_request.spec_.spec_id_;
  l_base.iteration_        = in_request.iteration_;
  l_base.candidate_index_  = 0;
  l_base.rig_id_           = uuid{};  // 由调用方一次性分配（接缝不落库，也不需要它唯一）
  l_base.baseline_level_   = in_request.baseline_level_;
  l_base.variant_axis_     = variant_axis_t{};
  l_base.ops_.clear();
  l_base.provenance_          = provenance_t{};
  l_base.provenance_.model_   = doodle_config::g_ark_inference_model;
  // prompt_hash_ 是「这份 rig 是从哪份输入推出来的」的审计指纹。用同一份 input_text 再算一次：
  // 这里没有别的随机源（seed_ 恒 0），所以同一输入必然得到同一指纹（§8.5 第 1 条）。
  l_base.provenance_.prompt_hash_ = std::to_string(std::hash<std::string>{}(build_candidates_input_text(in_request)));
  l_base.provenance_.seed_        = 0;

  // ---- 3) §9.1 的灵敏度前提：没有主光，两个最常用的轴会变成空操作 ----
  DOODLE_CHICK_HTTP(
      first_light_with_role(l_base, light_role::key) != nullptr, bad_gateway,
      "候选基线 rig 里没有 role=key 的主光：key_direction / key_fill_ratio 两个轴都会变成空操作，"
      "§9.1 要求的「候选之间真的在目标上有差异」无法成立"
  );

  // ---- 4) 轴计划：不合法就整体回退默认计划（不抛错）----
  iteration_axes_t l_axes{};
  std::string l_strategy{};
  if (const auto l_reason = parse_axis_plan(in_result_json, l_axes, l_strategy); l_reason.has_value()) {
    l_axes            = default_axis_plan(in_request.spec_);
    const auto l_head = l_strategy.empty() ? std::string{} : fmt::format("模型策略：{}｜", l_strategy);
    l_axes.strategy_  = fmt::format("{}模型给的轴计划被弃用：{}", l_head, *l_reason);
    if (l_axes.strategy_.size() > g_strategy_max_len) l_axes.strategy_.resize(g_strategy_max_len);
  } else {
    l_axes.strategy_ = l_strategy;
  }

  // ---- 5) OFAT 展开 ----
  auto l_rigs = expand_candidates(l_base, l_axes);

  // ---- 6) 自洽（契约 3）：钉死身份、写 ops_、确认每个变异候选真的与基线不同 ----
  for (std::size_t l_i = 0; l_i < l_rigs.size(); ++l_i) {
    auto& l_rig            = l_rigs.at(l_i);
    l_rig.session_id_      = in_request.session_id_;
    l_rig.spec_id_         = in_request.spec_.spec_id_;
    l_rig.iteration_       = in_request.iteration_;
    l_rig.baseline_level_  = in_request.baseline_level_;
    l_rig.candidate_index_ = static_cast<std::int32_t>(l_i);
    l_rig.rig_id_          = uuid{};
    l_rig.provenance_      = l_base.provenance_;
    l_rig.ops_             = diff_rigs(l_base, l_rig);
    if (l_i == 0) continue;
    // §9.1 的「候选真的在目标上有差异」的最小实现：沿某个轴变了、但 rig 一个字节都没变，
    // 说明这条轴在当前 rig 上不生效（缺 fill 灯、单位不可比、已经在夹取边界上……）。
    // 返回 6 张一样的图比一次失败更糟 —— 那会让美术以为「这几个方向看起来一样」。
    DOODLE_CHICK_HTTP(
        !(rig_substance(l_rig) == rig_substance(l_base)), bad_gateway,
        "候选 {} 沿轴 {} = {} 展开后与基线完全相同（这条轴在当前 rig 上不生效）",
        l_i, magic_enum::enum_name(l_rig.variant_axis_.axis_), l_rig.variant_axis_.value_
    );
  }

  candidate_generation_result_t l_ret{};
  l_ret.rigs_ = std::move(l_rigs);
  l_ret.axes_ = std::move(l_axes);
  // 契约 3 的另一半：调用方还会再校验一次（500），这里先保证自己自洽。
  DOODLE_CHICK_HTTP(
      l_ret.rigs_.size() == static_cast<std::size_t>(g_candidate_count), internal_server_error,
      "OFAT 展开后得到 {} 个候选，方案 §6.6 要求恰好 {}", l_ret.rigs_.size(), g_candidate_count
  );
  return l_ret;
}

boost::asio::awaitable<candidate_generation_result_t> generate_candidates(
    sqlite_database& in_sql, const candidate_generation_request_t& in_request
) {
  // ---- 1) 一次推理：触发人 → 工作室 → 客户端 → 任务信封 → 成败判定，全在 seam_common ----
  // `iteration_id_` 由调用方在调用**之前**铸造好，所以这条推理任务能反查到
  // 「哪一次推理生成了第 N 轮候选」。
  // 请求体先落地成局部变量：下面的诊断要把**同一份** schema 交给校验库（§3.4 明说不额外重建）。
  // 已经转成 json：run_seam_inference 收 json，且落库与出网是同一份（见 seam_common.h）。
  nlohmann::json l_request{};
  to_json(l_request, build_candidates_request(in_request));
  const auto l_result = co_await run_seam_inference(in_sql, in_request, l_request, in_request.iteration_id_);

  // ---- 2) 架构诊断（§3.4，决策 D-1：只写日志，不作准入闸门）----
  // 本条接缝是**严格**策略：`build_base_rig` 读到第一个缺字段/类型不符就抛 502。日志里的
  // 全部违规是排障时唯一能看到「模型到底哪里没按契约输出」的地方（抛出的消息只说第一个）。
  if (const auto l_bad =
          json_val::validate_model_output(l_result.json_, l_request.at("json_schema"), "generate_candidates");
      !l_bad.empty()) {
    SPDLOG_LOGGER_WARN(
        g_logger_ctrl().get_http(), "AI 灯光会话 {} 第 {} 轮候选：模型输出有 {} 处不符合 schema（只诊断、不拒绝）：{}",
        in_request.session_id_, in_request.iteration_, l_bad.size(), json_val::join_violations(l_bad)
    );
  }

  // ---- 3) 映射（物化基线 + 轴计划校验/回退 + OFAT 展开 + ops diff + 自洽检查）----
  auto l_ret = map_candidate_result(l_result.json_, in_request);

  SPDLOG_LOGGER_INFO(
      g_logger_ctrl().get_http(),
      "AI 灯光会话 {} 第 {} 轮候选生成完成：候选 {} 个，轴 {} 条（{}），token in/out/total={}/{}/{}",
      in_request.session_id_, in_request.iteration_, l_ret.rigs_.size(), l_ret.axes_.axes_.size(),
      l_ret.axes_.strategy_, l_result.input_tokens_, l_result.output_tokens_, l_result.total_tokens_
  );

  co_return l_ret;
}

}  // namespace doodle::ai_light
