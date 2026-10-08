//
// AI 灯光 · 接缝 1 实现：提示词（+ 参考图统计）→ LightingSpec
// （设计见 docs/ue_ai_lighting_plan.md §15.7）
//
// 离线可测部件（build_spec_instructions / build_spec_json_schema / build_spec_input_text /
// build_spec_request / map_spec_result）与联网入口 generate_spec 都定义在这里；
// 声明在 ai_light_seam_spec.h。
//

#include "ai_light_seam_spec.h"

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

// ORM 的构造助手都在 doodle::orm 里（本文件的聚合与映射暂不碰库，但保持与另两条接缝一致）。
using namespace orm;

// ===========================================================================
// 接缝 1：参考图统计 + 人工提示词 → LightingSpec（方案 §5.3 / §6.1 / §6.3）
// ===========================================================================
//
// 分两层，理由是可测性：单测宿主没有 SSL context（core_set::ctx_ptr）也没有 http logger，
// 真正联网的那一段跑不起来；所以「组信封 / 映射结果」全部做成纯函数（无网络、无数据库），
// 由 ai_light_spec_prompt.cpp 离线覆盖，generate_spec 只负责把两层串起来。

namespace {

/// 证据判定的浮点容差：LLM 把统计数字原样抄回来时不该因为最后一位误差被判成「改了值」。
constexpr double g_evidence_epsilon = 1e-9;

/// 下面的「自由文本枚举」在 spec.h 里是 std::string（取值写在注释里，不是 C++ 枚举）。
/// 把取值收成一处常量，**schema 与校验共用** —— 否则「schema 里写了 subtle、校验却不认它」
/// 这种漂移不会有任何编译期或运行期信号。
constexpr std::array<std::string_view, 4> g_grading_strength_values{"none", "subtle", "moderate", "strong"};
constexpr std::array<std::string_view, 3> g_hardness_values{"hard", "soft", "very_soft"};
constexpr std::array<std::string_view, 3> g_position_bias_values{"side_high", "front_low", "back_high"};
constexpr std::array<std::string_view, 3> g_kelvin_bias_values{"cool", "warm", "neutral"};

/// 模型必须在 evidence 里逐字段交代来源的字段路径 —— 与 map_spec_result 的 put_evidence()
/// 用的是同一套点分路径，将来若要合并模型的自我归因，两边键名天然对齐。
/// 只列**模型可写**的字段：`targets.palette` 与 `reference_stats` 是服务端口径（模型改不了
/// 它们），让模型为这些编理由没有意义。
constexpr std::array<std::string_view, 17> g_evidence_paths{
    "intent.atmosphere",        "intent.time_of_day",          "intent.weather",
    "intent.source_motivation", "intent.character_personality", "intent.story_context",
    "intent.setting",           "targets.contrast",            "targets.white_balance_kelvin",
    "targets.luma_percentiles.p10", "targets.luma_percentiles.p50", "targets.luma_percentiles.p90",
    "targets.clip_rate.high",   "targets.clip_rate.low",       "targets.grading_hints.vignette",
    "targets.grading_hints.lifted_blacks", "targets.grading_hints.bloom",
};

// 架构（schema）的**形状**建造器与取值器都在 ai_light_json_validation.h 的 json_val::：
// 三个接缝共用同一份定义，避免同名形状各写一遍（形状一致也便于统一改）。
// 取值器（is_one_of / opt_object / opt_string / opt_bool / opt_interval / opt_string_array）
// 同样在那儿（严格 / 宽松两族）。共用是必须的：错误消息里要带接缝名，
// 硬编码在某一处会让别的接缝报错措辞错人。


void put_evidence(
    std::map<std::string, evidence_entry_t>& in_out, const std::string& in_path, evidence_source in_source,
    double in_confidence, const std::string& in_note = {}
) {
  evidence_entry_t l_entry{};
  l_entry.source_     = in_source;
  l_entry.confidence_ = in_confidence;
  l_entry.note_       = in_note;
  in_out[in_path]     = l_entry;
}



}  // namespace


std::string build_spec_instructions() {
  // 共用段（职责边界 + 性格映射表 + 单位口径）只允许有一处定义：两份提示词对「对比度是
  // 什么」说法不一致，模型就会在两条接缝上给出互不相容的数值（见头文件的说明）。
  std::string l_out{
      "你是电影灯光指导。把一个人工提示词与多张参考图的统计，蒸馏成一份可执行的灯光目标（LightingSpec）。\n\n"
  };
  l_out += build_common_lighting_section();
  l_out += R"LIGHT(
【冲突规则：人工提示词优先于参考图】
参考只提供环境与场景的结构与数值，提示词提供意图与语义。
- 提示词与参考一致 → 直接合并，置信度提高。
- 提示词明确、参考冲突 → 提示词胜出。
- 提示词未提及、参考明确 → 参考胜出。
- 提示词未提及、参考冲突 → 该维度不要设为硬目标，把它列进 conflicts，交人工裁决。

【输出要求】
只输出符合给定 JSON Schema 的 JSON。不要输出解释文字，不要用 Markdown 代码块。
调色板（palette）与 ownership 不在 schema 里：颜色只能来自参考图统计，不接受你编造数值。
)LIGHT";
  return l_out;
}

nlohmann::json build_spec_json_schema() {
  const auto l_interval     = json_val::schema_fixed_number_array(2);
  const auto l_string_array = json_val::schema_string_array();

  // 每个 object 都走 json_val::schema_object()：它固定产出
  // {type:object, properties, required(= **全部**属性名), additionalProperties:false}。
  // 「全字段 required」是方舟严格结构化输出的要求（§15.7），所以自动生成 required 与手写等价；
  // 而手抄 required 的旧写法漏一个字段没有任何编译期信号，代价却是一次已经计费的推理 400。
  const auto l_language = json_val::schema_object({
      {"key_fill_ratio", l_interval},
      {"hardness", json_val::schema_string_enum(g_hardness_values)},
      {"position_bias", json_val::schema_string_enum(g_position_bias_values)},
      {"kelvin_bias", json_val::schema_string_enum(g_kelvin_bias_values)},
  });

  const auto l_personality = json_val::schema_object({
      {"subject", json_val::schema_string()},
      {"trait", json_val::schema_string()},
      {"lighting_language", l_language},
  });

  const auto l_intent = json_val::schema_object({
      {"atmosphere", l_string_array},
      {"time_of_day", json_val::schema_string()},
      {"weather", json_val::schema_string()},
      {"source_motivation", l_string_array},
      {"character_personality", json_val::schema_array(l_personality)},
      {"story_context", json_val::schema_string()},
      {"setting", json_val::schema_string()},
  });

  const auto l_luma = json_val::schema_object({
      {"p10", l_interval},
      {"p50", l_interval},
      {"p90", l_interval},
  });

  const auto l_clip = json_val::schema_object({
      {"high", l_interval},
      {"low", l_interval},
  });

  const auto l_grading = json_val::schema_object({
      {"vignette", json_val::schema_string_enum(g_grading_strength_values)},
      {"lifted_blacks", json_val::schema_bool()},
      {"bloom", json_val::schema_string_enum(g_grading_strength_values)},
  });

  // 刻意没有 palette / ownership / space：颜色与所有权划分是服务端的口径，
  // 让模型每个字段都能自由填只会引入无法校验的分叉（§6.6 护栏）。
  const auto l_targets = json_val::schema_object({
      {"contrast", l_interval},
      {"white_balance_kelvin", l_interval},
      {"luma_percentiles", l_luma},
      {"clip_rate", l_clip},
      {"grading_hints", l_grading},
  });

  const auto l_evidence_entry = json_val::schema_object({
      {"source", json_val::schema_enum<evidence_source>()},
      {"confidence", json_val::schema_number(0.0, 1.0)},
      {"note", json_val::schema_string()},
  });

  // evidence 是**固定键**字典，键就是 g_evidence_paths 那 17 条字段路径。
  // 不能用 `additionalProperties = <条目 schema>` 那种自由字典：严格结构化输出要求每个
  // object 都 additionalProperties:false，自由字典会被判成不合规（这类 schema 最典型的
  // 真机 400）。而 evidence 的键本来就是有限集合（可写字段就那么些），固定键不丢表达力。
  nlohmann::json l_evidence_props = nlohmann::json::object();
  for (auto&& l_path : g_evidence_paths) l_evidence_props[std::string{l_path}] = l_evidence_entry;
  const auto l_evidence = json_val::schema_object(std::move(l_evidence_props));

  // 全字段 required + strict：OpenAI / 方舟的严格结构化输出基本都要求 required 列全。
  // 代价是「不设目标」不能用「省略字段」表达 —— 那种情况请把该维度写进 conflicts，
  // 并在对应字段上照抄统计值（人工闸门 1 看到的正是这份 conflicts）。
  // 真机若仍 400，§15.2 的降级方案是换成 SDK 的嵌套 text.format 形状。
  return json_val::schema_object({
      {"intent", l_intent},
      {"targets", l_targets},
      {"evidence", l_evidence},
      {"conflicts", l_string_array},
  });
}

std::string build_spec_input_text(
    const spec_generation_request_t& in_request, const aggregated_reference_stats_t& in_aggregated
) {
  std::string l_out{};

  l_out += "【人工提示词（权威意图）】\n";
  if (in_request.prompt_text_.empty()) {
    l_out += "(美术未填写提示词 —— 只能依据参考图统计推断；请把 intent 里靠猜的字段置信度调低)\n";
  } else {
    l_out += in_request.prompt_text_;
    l_out += "\n";
  }

  const auto l_excluded =
      static_cast<std::size_t>(std::count_if(in_request.references_.begin(), in_request.references_.end(), [](const auto& in_ref) {
        return in_ref.is_outlier_;
      }));
  l_out += fmt::format(
      "\n【参考图逐图统计（显示空间 sRGB）】共 {} 张，其中 {} 张已被人工排除"
      "（不进聚合，也不发图给你；仅在此列出以便你理解被否掉的是什么）\n",
      in_request.references_.size(), l_excluded
  );
  for (const auto& l_ref : in_request.references_) {
    const auto l_stats = l_ref.get_stats();
    l_out += fmt::format(
        "- #{} {}x{} confidence={:.2f}{}\n", l_ref.ordinal_, l_ref.width_, l_ref.height_, l_ref.stats_confidence_,
        l_ref.is_outlier_ ? fmt::format("  已人工排除（{}）", l_ref.excluded_reason_) : std::string{}
    );
    // 判据与匿名命名空间的 references_have_stats() 一致：get_stats() 返回的是值类型，
    // 统计缺失时得到的是全零的 reference_image_stats_t，无法与「真的全零」区分，
    // 所以可用性只能由 stats_ 是否为 null 与 stats_confidence_ 共同判定。
    if (l_ref.stats_.is_null() || l_ref.stats_confidence_ <= 0.0) {
      l_out += "  该图没有可用统计（分析失败或未分析）\n";
      continue;
    }
    l_out += fmt::format(
        "  luma p10={:.4f} p50={:.4f} p90={:.4f}  contrast={:.4f}  K={:.0f}\n",
        l_stats.luma_percentiles_.p10_, l_stats.luma_percentiles_.p50_, l_stats.luma_percentiles_.p90_, l_stats.contrast_,
        l_stats.white_balance_kelvin_
    );
    l_out += fmt::format(
        "  clip high={:.4f} low={:.4f}  retouch={:.2f}\n", l_stats.clip_rate_.high_, l_stats.clip_rate_.low_,
        l_stats.retouch_score_
    );
    if (!l_stats.palette_.empty()) {
      l_out += "  主色（按占比降序，前 3）: ";
      const auto l_take = std::min<std::size_t>(std::size_t{3}, l_stats.palette_.size());
      for (std::size_t l_i = 0; l_i < l_take; ++l_i)
        l_out += fmt::format("{}{:.0f}%", rgb_to_hex(l_stats.palette_[l_i].color_), l_stats.palette_[l_i].share_ * 100.0);
      l_out += "\n";
    }
  }

  l_out += "\n【聚合目标（多图中位数 + 极差区间）】\n";
  if (in_aggregated.stats_.per_dimension_confidence_.empty()) {
    l_out += "没有可用统计 —— 请完全依据提示词推断 targets，不要编造精确定位数字。\n";
  } else {
    l_out += describe_targets(in_aggregated.targets_);
    l_out += fmt::format(
        "  参与聚合的图 {} 张，聚合方式 {}（区间取极差，不是均值±容差）\n", in_aggregated.stats_.image_count_,
        in_aggregated.stats_.aggregation_
    );
    l_out += "  逐维置信度:";
    for (const auto& [l_key, l_value] : in_aggregated.stats_.per_dimension_confidence_)
      l_out += fmt::format(" {}={:.2f}", l_key, l_value);
    l_out += "\n";
    if (!in_aggregated.stats_.conflicts_.empty())
      l_out += fmt::format(
          "  冲突维度（参考图之间互相矛盾 → 列进 conflicts，不要当硬目标）: {}\n",
          join_strings(in_aggregated.stats_.conflicts_, ", ")
      );
    if (!in_aggregated.suspected_outlier_ids_.empty()) {
      l_out += "  疑似外点（仅供你判断参考，尚未排除）: ";
      for (const auto& l_id : in_aggregated.suspected_outlier_ids_) l_out += boost::uuids::to_string(l_id) + " ";
      l_out += "\n";
    }
    l_out += "  调色板（只能照抄，不接受改写）: ";
    for (const auto& l_entry : in_aggregated.targets_.palette_)
      l_out += fmt::format(
          "{}[{:.0f}%-{:.0f}%] ", rgb_to_hex(l_entry.color_), l_entry.share_.low_ * 100.0, l_entry.share_.high_ * 100.0
      );
    l_out += "\n";
  }

  if (in_request.previous_spec_.has_value()) {
    l_out += "\n【上一版方案（美术对上一版不满意，请重出）】\n";
    l_out += "上一版 intent:\n";
    l_out += fmt::format(
        "  atmosphere=[{}] time_of_day={} weather={}\n  source_motivation=[{}] setting={}\n  story_context={}\n",
        join_strings(in_request.previous_spec_->intent_.atmosphere_, ", "),
        in_request.previous_spec_->intent_.time_of_day_, in_request.previous_spec_->intent_.weather_,
        join_strings(in_request.previous_spec_->intent_.source_motivation_, ", "),
        in_request.previous_spec_->intent_.setting_, in_request.previous_spec_->intent_.story_context_
    );
    l_out += "上一版 targets:\n";
    l_out += describe_targets(in_request.previous_spec_->targets_);
    l_out +=
        "除非提示词或统计明确要求改动，否则请与上一版保持一致 —— 美术不满意的是具体某处，"
        "大范围重写会让上一轮的判断信息作废。\n";
  }

  return l_out;
}

http::seedance2::inference_request_t build_spec_request(
    const spec_generation_request_t& in_request, const aggregated_reference_stats_t& in_aggregated
) {
  // 直接填强类型信封：键名与类型只在 seedance2_inference_request.h 一处定义,
  // 消费端 build_inference_body() 按字段读, 两侧改名/改类型会在编译期暴露。
  http::seedance2::inference_request_t l_req{};
  l_req.instructions_       = build_spec_instructions();
  l_req.input_text_         = build_spec_input_text(in_request, in_aggregated);
  l_req.image_detail_       = "high";
  l_req.temperature_        = 0.2;
  l_req.json_schema_        = build_spec_json_schema();
  l_req.json_schema_name_   = "lighting_spec";
  l_req.json_schema_strict_ = true;
  // 刻意**不设** max_output_tokens：schema 要求全字段 required（含 17 条 evidence），输出长度
  // 不可预估，一旦被截断 result_json_ 就变成不可解析的半个 JSON —— 那个失败面与「平铺形状
  // 被拒」混在一起根本分不出来。上限交给模型/账号侧决定。
  // 不设 thinking：缺省就是 disabled，而结构化任务开 thinking 会引入
  // 「output[0] 是 reasoning 项」那条坑（§15.6 第 2 条）。

  // images 填**站点内相对路径**，由 to_public_url 补公网前缀 + 短时效签名；
  // 绝对磁盘路径（file_path_）绝不能外发。被人工排除的图不发。
  // 空 vector 不会写 images 键（to_json 保证），即「不发 images 空数组」。
  for (const auto& l_ref : in_request.references_) {
    if (l_ref.is_outlier_) continue;
    // reference_image_url 定义在路由头 ai_light.h 的 doodle::http::ai_light 命名空间里。
    l_req.images_.emplace_back(http::ai_light::reference_image_url(l_ref.uuid_id_));
  }
  return l_req;
}

spec_generation_result_t map_spec_result(
    const nlohmann::json& in_result_json, const aggregated_reference_stats_t& in_aggregated
) {
  DOODLE_CHICK_HTTP(
      in_result_json.is_object(), bad_gateway, "LLM 未返回 JSON 对象（{}），无法映射为 LightingSpec",
      in_result_json.is_null() ? std::string{"null —— 没有按 json_schema 输出"} : in_result_json.type_name()
  );

  spec_generation_result_t l_ret{};
  auto& l_spec = l_ret.spec_;

  // ---- 服务端钉死：模型无权决定的部分 ----
  // schema 里根本没有这些键，所以「钉死」= 不读模型输出，只写服务端口径。
  // 身份字段（spec_id_ / session_id_）由调用方填：spec_id_ 派生自 uuid_id_，
  // session_id_ 是会话锚点，都不属于 LLM 的契约。
  l_spec.schema_version_ = std::string{g_spec_schema_version};
  l_spec.targets_        = in_aggregated.targets_;  // 区间 + palette + ownership 全来自统计
  l_spec.targets_.space_ = "display_srgb";
  l_ret.reference_stats_ = in_aggregated.stats_;
  l_spec.reference_stats_ = in_aggregated.stats_;
  l_spec.human_confirmed_ = false;
  l_spec.confirmed_by_    = uuid{};
  l_spec.confirmed_at_    = std::nullopt;

  // 降级级别以**事实**为准：聚合器一个维度的置信度都没给出 = 没有可用统计。
  // 不用调用方那个 references_have_stats() 的预判 —— 它只看 stats_confidence_ > 0，
  // 而「全部图被人工排除」「stats 行存在但都是空壳」都会在这里翻案（决定 D4）。
  const bool l_has_stats = !in_aggregated.stats_.per_dimension_confidence_.empty();
  l_spec.degrade_level_  = l_has_stats ? spec_degrade_level::stats_only : spec_degrade_level::prompt_only;

  put_evidence(
      l_ret.evidence_, "targets.palette", evidence_source::reference_stats, l_has_stats ? 0.8 : 0.3,
      "调色板只来自参考图统计，不接受模型编造"
  );
  put_evidence(
      l_ret.evidence_, "reference_stats", evidence_source::reference_stats,
      in_aggregated.stats_.per_dimension_confidence_.empty()
          ? 0.0
          : [&] {
              double l_sum = 0.0;
              for (const auto& [l_key, l_value] : in_aggregated.stats_.per_dimension_confidence_) l_sum += l_value;
              return l_sum / static_cast<double>(in_aggregated.stats_.per_dimension_confidence_.size());
            }(),
      fmt::format("{} 张图，聚合方式 {}", in_aggregated.stats_.image_count_, in_aggregated.stats_.aggregation_)
  );

  // ---- 模型可覆盖：先记下被改动的维度，供 §6.3 的裁决留痕 ----
  std::vector<std::string> l_overridden{};

  // contrast 等区间的 evidence 判定是**机械的**：与统计同名字段逐元素比较，
  // 相等 → 模型只是把统计抄了回来（reference_stats）；不等 → 按「提示词优先」归因于提示词。
  const auto l_apply_interval =
      [&](const nlohmann::json& in_src, const char* in_key, interval_t& in_out, const std::string& in_path) {
        const auto l_llm = json_val::opt_interval(in_src, in_key);
        if (!l_llm.has_value()) return;
        const bool l_same = std::abs(l_llm->low_ - in_out.low_) < g_evidence_epsilon &&
                            std::abs(l_llm->high_ - in_out.high_) < g_evidence_epsilon;
        in_out = *l_llm;
        if (l_same) {
          put_evidence(l_ret.evidence_, in_path, evidence_source::reference_stats, 0.8, "与参考图统计一致");
        } else {
          put_evidence(
              l_ret.evidence_, in_path, evidence_source::prompt, 0.7,
              "与参考图统计不一致：按 §6.3「提示词优先于参考」采用模型给出的值"
          );
          l_overridden.emplace_back(in_path);
        }
      };

  if (const auto l_targets_json = json_val::opt_object(in_result_json, "targets"); l_targets_json.has_value()) {
    l_apply_interval(*l_targets_json, "contrast", l_spec.targets_.contrast_, "targets.contrast");
    l_apply_interval(
        *l_targets_json, "white_balance_kelvin", l_spec.targets_.white_balance_kelvin_, "targets.white_balance_kelvin"
    );
    if (const auto l_luma = json_val::opt_object(*l_targets_json, "luma_percentiles"); l_luma.has_value()) {
      l_apply_interval(*l_luma, "p10", l_spec.targets_.luma_percentiles_.p10_, "targets.luma_percentiles.p10");
      l_apply_interval(*l_luma, "p50", l_spec.targets_.luma_percentiles_.p50_, "targets.luma_percentiles.p50");
      l_apply_interval(*l_luma, "p90", l_spec.targets_.luma_percentiles_.p90_, "targets.luma_percentiles.p90");
    }
    if (const auto l_clip = json_val::opt_object(*l_targets_json, "clip_rate"); l_clip.has_value()) {
      l_apply_interval(*l_clip, "high", l_spec.targets_.clip_rate_.high_, "targets.clip_rate.high");
      l_apply_interval(*l_clip, "low", l_spec.targets_.clip_rate_.low_, "targets.clip_rate.low");
    }
    if (const auto l_grading = json_val::opt_object(*l_targets_json, "grading_hints"); l_grading.has_value()) {
      // 自由文本枚举：**必须自己校验**。裸 get<std::string>() 不会拦下 "super_hard"，
      // 而 grading_hints 直接进调色参数，非法值会一路传到下游。
      // opt_one_of = 「读字符串 + 查白名单」一步，白名单与 schema 用的是同一份常量。
      if (const auto l_v = json_val::opt_one_of(*l_grading, "vignette", g_grading_strength_values);
          l_v.has_value()) {
        l_spec.targets_.grading_hints_.vignette_ = *l_v;
        put_evidence(l_ret.evidence_, "targets.grading_hints.vignette", evidence_source::prompt, 0.6);
      } else if (const auto l_raw = json_val::opt_string(*l_grading, "vignette"); l_raw.has_value()) {
        // opt_one_of 已经把白名单外的取值滤成 nullopt，这里只负责把「模型给了什么」留痕。
        put_evidence(
            l_ret.evidence_, "targets.grading_hints.vignette", evidence_source::default_value, 0.3,
            fmt::format("模型给出的取值 \"{}\" 不在允许集合内，已回退为统计推导值", *l_raw)
        );
      }
      if (const auto l_v = json_val::opt_bool(*l_grading, "lifted_blacks"); l_v.has_value()) {
        l_spec.targets_.grading_hints_.lifted_blacks_ = *l_v;
        put_evidence(l_ret.evidence_, "targets.grading_hints.lifted_blacks", evidence_source::prompt, 0.6);
      }
      if (const auto l_v = json_val::opt_one_of(*l_grading, "bloom", g_grading_strength_values); l_v.has_value()) {
        l_spec.targets_.grading_hints_.bloom_ = *l_v;
        put_evidence(l_ret.evidence_, "targets.grading_hints.bloom", evidence_source::prompt, 0.6);
      } else if (const auto l_raw = json_val::opt_string(*l_grading, "bloom"); l_raw.has_value()) {
        put_evidence(
            l_ret.evidence_, "targets.grading_hints.bloom", evidence_source::default_value, 0.3,
            fmt::format("模型给出的取值 \"{}\" 不在允许集合内，已回退为统计推导值", *l_raw)
        );
      }
    }
  }

  if (const auto l_intent_json = json_val::opt_object(in_result_json, "intent"); l_intent_json.has_value()) {
    l_spec.intent_.atmosphere_        = json_val::opt_string_array(*l_intent_json, "atmosphere");
    l_spec.intent_.time_of_day_       = json_val::opt_string(*l_intent_json, "time_of_day").value_or("");
    l_spec.intent_.weather_           = json_val::opt_string(*l_intent_json, "weather").value_or("");
    l_spec.intent_.source_motivation_ = json_val::opt_string_array(*l_intent_json, "source_motivation");
    l_spec.intent_.story_context_     = json_val::opt_string(*l_intent_json, "story_context").value_or("");
    l_spec.intent_.setting_           = json_val::opt_string(*l_intent_json, "setting").value_or("");

    if (const auto l_it = l_intent_json->find("character_personality");
        l_it != l_intent_json->end() && l_it->is_array()) {
      for (const auto& l_p : *l_it) {
        if (!l_p.is_object()) continue;
        character_personality_t l_cp{};
        l_cp.subject_ = json_val::opt_string(l_p, "subject").value_or("");
        l_cp.trait_   = json_val::opt_string(l_p, "trait").value_or("");
        if (const auto l_ll = json_val::opt_object(l_p, "lighting_language"); l_ll.has_value()) {
          if (const auto l_kf = json_val::opt_interval(*l_ll, "key_fill_ratio"); l_kf.has_value())
            l_cp.lighting_language_.key_fill_ratio_ = l_kf;
          // opt_one_of = 「读字符串 + 查白名单」一步：白名单外的取值直接变 nullopt，
          // 于是「值非法」与「没给」在这里合流，二者都保留默认值、都由 else 分支留痕。
          if (const auto l_v = json_val::opt_one_of(*l_ll, "hardness", g_hardness_values); l_v.has_value())
            l_cp.lighting_language_.hardness_ = *l_v;
          else if (const auto l_raw = json_val::opt_string(*l_ll, "hardness"); l_raw.has_value())
            put_evidence(
                l_ret.evidence_, "intent.character_personality.lighting_language.hardness",
                evidence_source::default_value, 0.3, fmt::format("非法取值 \"{}\" 已丢弃", *l_raw)
            );
          if (const auto l_v = json_val::opt_one_of(*l_ll, "position_bias", g_position_bias_values); l_v.has_value())
            l_cp.lighting_language_.position_bias_ = *l_v;
          else if (const auto l_raw = json_val::opt_string(*l_ll, "position_bias"); l_raw.has_value())
            put_evidence(
                l_ret.evidence_, "intent.character_personality.lighting_language.position_bias",
                evidence_source::default_value, 0.3, fmt::format("非法取值 \"{}\" 已丢弃", *l_raw)
            );
          if (const auto l_v = json_val::opt_one_of(*l_ll, "kelvin_bias", g_kelvin_bias_values); l_v.has_value())
            l_cp.lighting_language_.kelvin_bias_ = *l_v;
          else if (const auto l_raw = json_val::opt_string(*l_ll, "kelvin_bias"); l_raw.has_value())
            put_evidence(
                l_ret.evidence_, "intent.character_personality.lighting_language.kelvin_bias",
                evidence_source::default_value, 0.3, fmt::format("非法取值 \"{}\" 已丢弃", *l_raw)
            );
        }
        // 主语和性格都空的行没有信息量，丢掉而不是留一堆空壳。
        if (!l_cp.subject_.empty() || !l_cp.trait_.empty())
          l_spec.intent_.character_personality_.emplace_back(std::move(l_cp));
      }
    }

    if (!l_spec.intent_.atmosphere_.empty())
      put_evidence(l_ret.evidence_, "intent.atmosphere", evidence_source::prompt, 0.8);
    if (!l_spec.intent_.character_personality_.empty())
      put_evidence(
          l_ret.evidence_, "intent.character_personality", evidence_source::prompt, 0.7,
          "按 §5.5 的性格→灯光语言映射表落成 lighting_language"
      );
  }

  // ---- 冲突与冲突裁决 ----
  auto l_conflicts = json_val::opt_string_array(in_result_json, "conflicts");
  for (auto&& l_conflict : l_conflicts)
    l_ret.reference_stats_.conflicts_.emplace_back(fmt::format("llm: {}", l_conflict));

  l_ret.conflict_resolution_.resolved_["conflicts"]                 = in_aggregated.stats_.conflicts_;
  l_ret.conflict_resolution_.resolved_["per_dimension_confidence"]  = in_aggregated.stats_.per_dimension_confidence_;
  if (!in_aggregated.suspected_outlier_ids_.empty()) {
    // 只报不写：is_outlier_ 是人工排除字段，自动判定写进去会让两种语义在同一列里不可区分（决定 D3）。
    nlohmann::json l_outliers = nlohmann::json::array();
    for (const auto& l_id : in_aggregated.suspected_outlier_ids_) l_outliers.emplace_back(boost::uuids::to_string(l_id));
    l_ret.conflict_resolution_.resolved_["suspected_outliers"] = l_outliers;
  }
  if (!l_overridden.empty()) l_ret.conflict_resolution_.resolved_["overridden_dimensions"] = l_overridden;
  if (!l_conflicts.empty()) l_ret.conflict_resolution_.resolved_["llm_reported"] = l_conflicts;

  std::string l_note = in_aggregated.stats_.conflicts_.empty()
                           ? std::string{"参考图各维度一致：目标取多图中位数，区间取极差。"}
                           : fmt::format(
                                 "以下维度在参考图之间互相矛盾，未设为硬目标，需人工裁决：{}",
                                 join_strings(in_aggregated.stats_.conflicts_, ", ")
                             );
  if (!l_overridden.empty())
    l_note += fmt::format(
        " 提示词与参考不一致、按 §6.3 采用提示词值的维度：{}", join_strings(l_overridden, ", ")
    );
  l_ret.conflict_resolution_.note_ = l_note;

  // 返回值自洽（契约 3）：spec 里带着同一份 evidence，调用方无论读哪一份都一致。
  l_spec.evidence_ = l_ret.evidence_;
  return l_ret;
}

boost::asio::awaitable<spec_generation_result_t> generate_spec(
    sqlite_database& in_sql, const spec_generation_request_t& in_request
) {
  // ---- 1) 统计聚合（纯计算，无 IO；无可用统计也不抛，走 prompt_only 降级）----
  const auto l_agg = aggregate_reference_stats(in_request.references_);

  // ---- 2) 一次推理：触发人 → 工作室 → 客户端 → 任务信封 → 成败判定，全在 seam_common ----
  // 蒸馏阶段还没有轮次，任务行的 ai_light_iteration_id 留 nil。
  // 请求体先落地成局部变量：下面的诊断要把**同一份** schema 交给校验库（§3.4 明说不额外重建）。
  const auto l_request = build_spec_request(in_request, l_agg);
  const auto l_result =
      co_await run_seam_inference(in_sql, in_request, "generate_spec", in_request.prompt_text_, l_request);

  // ---- 3) 架构诊断（§3.4，决策 D-1：只写日志，不作准入闸门）----
  // spec 接缝**有意**容忍截断/不完整输出（靠统计默认值 + evidence.source=default_value 留痕），
  // 一道 schema 闸门会直接破坏这个设计。所以这里报出违规也照常往下走：收益是日志里能看到
  // **全部**违规（宽松读取器读到坏字段只是跳过），而不是毫无信号。
  if (const auto l_bad = json_val::validate_model_output(l_result.json_, l_request.json_schema_, "generate_spec");
      !l_bad.empty()) {
    SPDLOG_LOGGER_WARN(
        g_logger_ctrl().get_http(), "AI 灯光会话 {} 蒸馏 Spec：模型输出有 {} 处不符合 schema（只诊断、不拒绝）：{}",
        in_request.session_id_, l_bad.size(), json_val::join_violations(l_bad)
    );
  }

  // ---- 4) 映射 ----
  auto l_ret = map_spec_result(l_result.json_, l_agg);
  // 身份字段由调用方（advance_session）负责写进 ai_light_spec，这里只保证自洽。
  l_ret.spec_.session_id_ = in_request.session_id_;

  SPDLOG_LOGGER_INFO(
      g_logger_ctrl().get_http(),
      "AI 灯光会话 {} 蒸馏 Spec 完成：降级 {}，参考图 {} 张（可用统计 {}），token in/out/total={}/{}/{}",
      in_request.session_id_, magic_enum::enum_name(l_ret.spec_.degrade_level_), in_request.references_.size(),
      l_agg.stats_.image_count_, l_result.input_tokens_, l_result.output_tokens_, l_result.total_tokens_
  );

  co_return l_ret;
}

}  // namespace doodle::ai_light
