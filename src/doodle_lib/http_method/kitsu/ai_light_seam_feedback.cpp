//
// AI 灯光 · 接缝 3 实现：自由文本反馈 → 结构化轴调整
// （设计见 docs/ue_ai_lighting_plan.md §15.9）
//
// 离线可测部件（build_feedback_* / map_feedback_result 及其文件内助手）与联网入口
// structure_feedback 都定义在这里；声明在 ai_light_seam_feedback.h。
//

#include "ai_light_seam_feedback.h"

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
// 接缝 3：自由文本反馈 → 结构化轴调整（方案 §6.2 / §13.1 第 9 项）
// ===========================================================================
//
// 与前两条接缝同样的分层：纯函数组信封/映射 + 一次真实网络调用。
// 产出比它们窄得多 —— 不是一套 rig、也不是一份方案，只是「往哪个方向动、动多少」。
// 这个窄产出正是「滑块是默认通道、文本是补充通道」的实现形态：两条通道最终写进
// 同一个 std::map<std::string,double>，再由 advance_session:307-319 变成 axis_step_t。

namespace {

/// 模型最多能给几条轴。**这不是审美选择而是硬约束**：下一轮的 parse_axis_plan() 只接受
/// 1..g_axis_max_count(2) 条，而 feedback_axes_ 是「这些轴必须出现在你的 axes 里」的
/// **约束** —— 给 3 条，模型无论如何都满足不了，轴计划必然被弃用，文本通道静默失效。
constexpr std::size_t g_feedback_axis_max_count = g_axis_max_count;

/// 一个方向的文字说明（reason）会作为 axis_step_t::label_ 落库并回显给美术。
/// 它是 TEXT 列，模型可以长篇大论，截断避免一次输出撑爆这一列。
constexpr std::size_t g_feedback_label_max_len = 200;

/// 6.000 -> "6"，0.350 -> "0.35"，-800.000 -> "-800"。
/// 给模型和美术看的数字不该带尾随零：提示词里写 "key:fill = 6.000" 会让人以为那是测量值。
std::string trim_number(double in_value) {
  std::string l_out = fmt::format("{:.3f}", in_value);
  if (l_out.find('.') == std::string::npos) return l_out;
  l_out.erase(l_out.find_last_not_of('0') + 1);
  if (!l_out.empty() && l_out.back() == '.') l_out.pop_back();
  return l_out;
}

}  // namespace

std::string build_feedback_instructions() {
  std::string l_out{};
  l_out +=
      "你是电影灯光指导的**翻译**环节：美术刚看完一轮渲染，用一句大白话说了他想要什么。\n"
      "你要把这句大白话翻译成灯光轴上的**方向**，由下一轮的排轴环节把它变成具体的档位。\n"
      "你**不**负责出灯、也**不**负责出档位表 —— 只判断「往哪个轴、往哪边、动多少」。\n\n";
  // 与 apply_axis_step 同一份口径（§15.9）：模型按它给方向，服务端按同一套公式应用。
  l_out += "【轴值语义】\n";
  l_out += g_axis_value_semantics_text;
  l_out += "\n【约束】\n";
  l_out += fmt::format(
      "- 每条轴只给**一个**数值（value）：这是下一轮的中心值，不是档位表。\n"
      "- 最多给 {} 条轴。下一轮只接受不超过 {} 条 —— 给多了整份轴计划会被丢弃，你的翻译就白做了。\n"
      "- 不要给 none（那不是方向）；不要给 0（0 等于「没有变化」，同样读不出方向）。\n"
      "- key_fill_ratio 是**绝对**比值，必须参考下面给出的「当前 key:fill 比」来定新值；\n"
      "  其余五个轴都是**增量**，正负决定方向。\n"
      "- 一个方向只由一条轴承担：别让两条轴说同一件事（比如既抬环境光又压对比）。\n"
      "- 美术没有给出任何方向时（只是说「再调调看」「换一批」），axes 就给**空数组** ——\n"
      "  不要为了凑数编一条，那会让下一轮往一个错的方向收缩。\n\n"
      "【输出要求】\n"
      "只输出符合给定 JSON Schema 的 JSON。不要输出解释文字，不要用 Markdown 代码块。\n"
      "understanding 用一句中文说清你的理解（给人工核对用）。\n",
      g_feedback_axis_max_count, g_feedback_axis_max_count
  );
  return l_out;
}

nlohmann::json build_feedback_json_schema() {
  const auto l_axis_item = json_val::schema_object({
      {"axis", json_val::schema_enum<variant_axis_kind>(variant_axis_kind::none)},
      {"value", json_val::schema_number()},
      {"reason", json_val::schema_string()},
  });

  // 只设上限、**不设 minItems**：空数组是「美术没给出方向」的合法表达（决定 D13），
  // 加了 minItems 会逼模型编一条不存在的方向。
  const auto l_axes = json_val::schema_array(l_axis_item, std::nullopt, g_feedback_axis_max_count);

  // description 只是给人看的说明，不影响形状，所以建好之后再补一句。
  auto l_understanding           = json_val::schema_string();
  l_understanding["description"] = "你对美术这句话的理解，一句话";

  return json_val::schema_object({
      {"understanding", l_understanding},
      {"axes", l_axes},
  });
}

std::string build_feedback_input_text(const feedback_structuring_request_t& in_request) {
  std::string l_out{};
  // ---- 1) 美术的原话（逐字）----
  l_out += "【美术的原话（权威，一字不改）】\n";
  l_out += in_request.feedback_text_;
  l_out += "\n";

  // ---- 2) 这次判断的决定 + 选中的候选 ----
  // 选中候选的**序号**（candidate_index_，从 0 开始）而不是 uuid：原话里的「第 3 个」
  // 指的就是界面上的排序，模型与美术必须看到同一个编号。
  std::int32_t l_selected_index = -1;
  if (!in_request.selected_candidate_id_.is_nil())
    for (const auto& l_cand : in_request.candidates_)
      if (l_cand.uuid_id_ == in_request.selected_candidate_id_) l_selected_index = l_cand.candidate_index_;

  l_out += "\n【这次判断的决定】\n";
  l_out += fmt::format("决定 = {}", magic_enum::enum_name(in_request.decision_));
  if (l_selected_index >= 0) {
    l_out += fmt::format("；美术选中了候选 #{}", l_selected_index);
  } else if (!in_request.selected_candidate_id_.is_nil()) {
    l_out += fmt::format("；美术选中了候选 {}（不在这轮的候选列表里）", in_request.selected_candidate_id_);
  } else {
    l_out += "；美术没有指定候选";
  }
  l_out += "\n";

  // ---- 3) Spec 目标（与前两条接缝同一个投影函数）----
  l_out += "\n【已确认的灯光方案（LightingSpec，人工已确认）】\n";
  l_out += fmt::format("降级级别 = {}\n", magic_enum::enum_name(in_request.spec_.degrade_level_));
  l_out += describe_spec_brief(in_request.spec_);
  l_out += "\n";

  // ---- 4) 本轮 6 个候选的对照结构 ----
  // OFAT 不变量（§15.8）：候选 0 是基线，其余各自只沿一条轴变一个档位 ——
  // 所以「基线 rig + 每个候选的轴档位」足以描述全部 6 张图，不必展开 6 套完整 rig
  // （那会把提示词撑大 6 倍，而多出来的信息只有重复）。
  l_out += fmt::format("\n【本轮（第 {} 轮）这 6 个候选是怎么来的】\n", in_request.iteration_);
  const lighting_rig_t l_base_rig =
      in_request.candidates_.empty() ? lighting_rig_t{} : in_request.candidates_.front().get_rig();
  if (in_request.candidates_.empty()) {
    l_out += "本轮候选还没落库。请只依据 Spec 与美术的原话判断方向。\n";
  } else {
    l_out += fmt::format("#0 基线：{}\n", describe_rig_brief(l_base_rig));
    for (const auto& l_cand : in_request.candidates_) {
      if (l_cand.candidate_index_ == 0) continue;
      const auto& l_axis = l_cand.get_rig().variant_axis_;
      l_out += fmt::format(
          "#{}  轴={}  值={}  {}\n", l_cand.candidate_index_, magic_enum::enum_name(l_axis.axis_),
          trim_number(l_axis.value_), l_axis.label_
      );
    }
  }

  // ---- 5) 选中候选的完整 rig ----
  if (l_selected_index >= 0) {
    for (const auto& l_cand : in_request.candidates_) {
      if (l_cand.candidate_index_ != l_selected_index) continue;
      l_out += "\n【美术选中的那套（原话最可能指的就是它）】\n";
      l_out += describe_rig_brief(l_cand.get_rig());
      l_out += "\n";
      break;
    }
  }

  // ---- 6) 当前 key:fill 比（绝对量轴必须知道现状）----
  if (!in_request.candidates_.empty()) {
    const auto l_ratio = key_fill_ratio_of(l_base_rig);
    l_out += "\n【当前 key:fill 比】\n";
    if (l_ratio.has_value()) {
      l_out += fmt::format("{}（基线 rig 的主光强度 ÷ 全部补光强度之和）\n", trim_number(*l_ratio));
    } else {
      l_out += "算不出来（缺主光 / 缺补光 / 主光与补光强度单位不同）—— 这时不要给 key_fill_ratio 轴。\n";
    }
  }

  // ---- 7) 滑块已经给出的方向 ----
  l_out += "\n【美术已经用滑块给出的方向】\n";
  if (in_request.slider_axes_.empty()) {
    l_out += "没有（美术只写了字）。\n";
  } else {
    for (const auto& [l_name, l_value] : in_request.slider_axes_)
      l_out += fmt::format("{} = {}\n", l_name, trim_number(l_value));
    // 决策 5 + §15.9 决定 D9：滑块是默认通道，同一轴上滑块优先。
    l_out += "这些轴美术已经自己拖过了，**不要再给**（给了也会被滑块的数值覆盖）。\n";
  }
  l_out += "\n";
  return l_out;
}

http::seedance2::inference_request_t build_feedback_request(const feedback_structuring_request_t& in_request) {
  http::seedance2::inference_request_t l_req{};
  l_req.instructions_       = build_feedback_instructions();
  l_req.input_text_         = build_feedback_input_text(in_request);
  l_req.temperature_        = 0.2;
  l_req.json_schema_        = build_feedback_json_schema();
  l_req.json_schema_name_   = "lighting_feedback";
  l_req.json_schema_strict_ = true;
  // 刻意不发（与 build_candidates_request 同样的三条理由）：
  //   * images / image_detail —— 固定这一轮的候选图在浏览器里，没进本请求；没有 images
  //     还发 image_detail 就是给「不存在的输入」配参数（§15.6 坑 21）。
  //   * max_output_tokens —— 缺省 4096 会截断，而截断的表现（result_json_ 变 null）
  //     与「schema 被拒」无法区分。
  //   * thinking —— 缺省 disabled；结构化任务开 thinking 会让 output[0] 变成 reasoning。
  return l_req;
}

std::vector<axis_step_t> map_feedback_result(const nlohmann::json& in_result_json) {
  // ---- 形状违规：抛错（协议被无视）----
  DOODLE_CHICK_HTTP(
      in_result_json.is_object(), bad_gateway, "文本反馈结构化的模型输出不是 JSON 对象：{}",
      in_result_json.is_null() ? std::string{"null"} : in_result_json.dump().substr(0, 200)
  );
  DOODLE_CHICK_HTTP(
      in_result_json.contains("understanding") && in_result_json.at("understanding").is_string(), bad_gateway,
      "文本反馈结构化的模型输出缺 understanding（或它不是字符串）"
  );
  DOODLE_CHICK_HTTP(
      in_result_json.contains("axes") && in_result_json.at("axes").is_array(), bad_gateway,
      "文本反馈结构化的模型输出缺 axes（或它不是数组）"
  );
  const auto& l_axes = in_result_json.at("axes");
  DOODLE_CHICK_HTTP(
      l_axes.size() <= g_feedback_axis_max_count, bad_gateway,
      "文本反馈结构化给了 {} 条轴，超过下一轮能接受的 {} 条", l_axes.size(), g_feedback_axis_max_count
  );

  // ---- 语义无效：在源头丢弃 ----
  // 为什么不抛错：unknown 轴名与 advance_session:314 的既有容错一致；none / 0 是「没有变化」
  // 的合法语义，但它一旦传下去，下一轮 advance_session 会把它变成 axis_step_t，
  // apply_axis_step() 对 0/none 恒返回 false，最终以「轴无实质变化」抛 502 ——
  // 那时病根与表现之间隔了一次**人工判断**（美术得先白判一轮）。丢弃在这里是最短的修复路径。
  //
  // 这里**不记日志**：本函数是离线可测的纯函数，而单测宿主没有 HTTP logger
  // （同 ai_light_stats.cpp 的 aggregate_reference_stats）。丢弃的条数由接缝本体
  // 用「模型给了几条 vs 我们留下几条」在日志里体现。
  std::vector<axis_step_t> l_out{};
  for (const auto& l_item : l_axes) {
    DOODLE_CHICK_HTTP(
        l_item.is_object() && l_item.contains("axis") && l_item.at("axis").is_string(), bad_gateway,
        "文本反馈结构化的 axes 元素缺 axis（或它不是字符串）"
    );
    const auto l_name = l_item.at("axis").get<std::string>();
    const auto l_kind = magic_enum::enum_cast<variant_axis_kind>(l_name);
    if (!l_kind.has_value() || *l_kind == variant_axis_kind::none) continue;
    // 同一条轴出现两次时取第一个：保持确定性，别让「最后一个赢」取决于模型输出顺序。
    const bool l_dup = std::any_of(
        l_out.begin(), l_out.end(), [&l_kind](const axis_step_t& in_s) { return in_s.axis_ == *l_kind; }
    );
    if (l_dup) continue;
    DOODLE_CHICK_HTTP(
        l_item.contains("value") && l_item.at("value").is_number(), bad_gateway,
        "文本反馈结构化的轴 {} 缺 value（或它不是数字）", l_name
    );
    const double l_value = l_item.at("value").get<double>();
    if (!std::isfinite(l_value) || l_value == 0.0) continue;
    // key_fill_ratio 是比值，负值/零没有意义（apply_axis_step 对 <= 0 也直接返回 false）。
    if (*l_kind == variant_axis_kind::key_fill_ratio && l_value <= 0.0) continue;

    axis_step_t l_step{};
    l_step.axis_ = *l_kind;
    // 每条轴**恰好一个值**（决定 D11）：调用方只取 front() 当下一轮的中心值，
    // 完整档位表由下一轮 generate_candidates 的 axes_ 决定。
    l_step.steps_ = {l_value};
    auto l_reason = std::string{};
    if (l_item.contains("reason") && l_item.at("reason").is_string()) l_reason = l_item.at("reason").get<std::string>();
    if (l_reason.empty()) l_reason = std::string{magic_enum::enum_name(*l_kind)};
    l_step.label_ = l_reason.substr(0, std::min<std::size_t>(g_feedback_label_max_len, l_reason.size()));
    l_out.emplace_back(std::move(l_step));
  }
  return l_out;
}

boost::asio::awaitable<std::vector<axis_step_t>> structure_feedback(
    sqlite_database& in_sql, const feedback_structuring_request_t& in_request
) {
  // ---- 1) 一次推理：触发人 → 工作室 → 客户端 → 任务信封 → 成败判定，全在 seam_common ----
  // `text_prompt_` 记的是「哪一轮的反馈被结构化了」，用于把一次计费对回一次人工判断。
  // 请求体先落地成局部变量：下面的诊断要把**同一份** schema 交给校验库（§3.4 明说不额外重建）。
  // 已经转成 json：run_seam_inference 收 json，且落库与出网是同一份（见 seam_common.h）。
  nlohmann::json l_request{};
  to_json(l_request, build_feedback_request(in_request));
  const auto l_result = co_await run_seam_inference(
      in_sql, in_request,
      fmt::format("AI 灯光会话 {} 第 {} 轮的文本反馈", in_request.session_id_, in_request.iteration_),
      l_request, in_request.iteration_id_
  );

  // ---- 2) 架构诊断（§3.4，决策 D-1：只写日志，不作准入闸门）----
  // 本条接缝是**混合**策略：形状违规（如 axes 超过 maxItems）在 map_feedback_result 里抛 502，
  // 语义无效的轴静默丢弃。日志里的全部违规能区分「模型多回了东西（无害）」与「模型回的结构
  // 真的不对（下一步就该收紧成闸门）」—— 后者才是决策 D-1 里那条一行改动的升级路径。
  if (const auto l_bad =
          json_val::validate_model_output(l_result.json_, l_request.at("json_schema"), "structure_feedback");
      !l_bad.empty()) {
    SPDLOG_LOGGER_WARN(
        g_logger_ctrl().get_http(), "AI 灯光会话 {} 第 {} 轮文本反馈：模型输出有 {} 处不符合 schema（只诊断、不拒绝）：{}",
        in_request.session_id_, in_request.iteration_, l_bad.size(), json_val::join_violations(l_bad)
    );
  }

  // ---- 3) 映射（形状违规抛错；语义无效的轴丢弃）----
  auto l_steps = map_feedback_result(l_result.json_);

  // 「模型给了几条 vs 我们留下几条」是唯一能观测到「轴被丢弃」的地方（map_feedback_result
  // 是纯函数、不记日志），所以这里连原始 axes 一起记下来。
  SPDLOG_LOGGER_INFO(
      g_logger_ctrl().get_http(),
      "AI 灯光会话 {} 第 {} 轮文本反馈结构化完成：模型给 {} 条轴、留下 {} 条，理解=\"{}\"，原始 axes={}，"
      "token in/out/total={}/{}/{}",
      in_request.session_id_, in_request.iteration_, l_result.json_.at("axes").size(), l_steps.size(),
      l_result.json_.value("understanding", std::string{}), l_result.json_.at("axes").dump(),
      l_result.input_tokens_, l_result.output_tokens_, l_result.total_tokens_
  );

  co_return l_steps;
}

}  // namespace doodle::ai_light
