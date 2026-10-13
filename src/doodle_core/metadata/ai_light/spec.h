//
// AI 灯光：灯光目标 LightingSpec（一等工件）
//
// 对应 docs/ue_ai_lighting_plan.md §4.2。参考不直接流向评判，而是先被蒸馏成一份
// 显式、可核对、可版本化、可回归测试的目标。
//
// 本文件里有两类东西，别混淆：
//   1) 纯数据结构（lighting_intent_t / lighting_targets_t / reference_stats_t / lighting_spec_t）
//      —— 供感知层、规划层、评判层在内存里传递，并序列化成 §4.2 的 JSON 形态。
//   2) 表实体 ai_light_spec —— 落库用。它把上面那份 JSON 拆成若干列：
//      可查询/可裁决的字段（revision / degrade_level / human_confirmed / confirmed_*）是真实列，
//      嵌套结构（intent / targets / reference_stats）是 JSON 列。
//

#pragma once

#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/metadata/ai_light/common.h>
#include <doodle_core/metadata/ai_light/rig.h>
#include <doodle_core/metadata/base.h>

#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace doodle::ai_light {

/// 降级等级（docs/ue_ai_lighting_plan.md §12.1）。
/// 原 §4.2 里同时有 tier 与 degrade_level；Tier B 取消后两者的取值集合等价
/// （原 degrade_level 的 full 值依赖 Tier B 的位姿反解，已不可达），
/// 所以本实现只保留 degrade_level 这一个字段，Spec schema 记为 1.4。
enum class spec_degrade_level {
  /// 多图统计聚合可用（默认，唯一主路径）
  stats_only,
  /// 参考不可用/完全冲突 → 只用提示词 + 预设库检索
  prompt_only,
};
DOODLE_JSON_SERIALIZE_ENUM_STRICT(
    spec_degrade_level, {
                            {spec_degrade_level::stats_only, "stats_only"},
                            {spec_degrade_level::prompt_only, "prompt_only"},
                        }
)

/// 意图：来自人工提示词，**权威**。与参考统计冲突时以本块为准（§6.3）。
///
/// ⚠️ 本块只剩 `weather_` 一个字段。原设计里它是一份「自然语言意图的结构化解释」——
/// atmosphere / time_of_day / source_motivation / story_context / setting，以及
/// character_personality（性格 → lighting_language 灯光语言）。那些字段**全部删除**了：
/// 它们的唯一生产者是已删的 generate_spec 接缝（让模型把提示词翻译成结构化字段），
/// 后端不再用大模型推理，就没有任何东西会产生它们 —— 留在结构里只会让
/// 「空数组 / 空字符串」看起来像「模型认为没有氛围」。
///
/// 提示词本身（`ai_light_session.prompt_text_`）才是权威意图的载体，前端把它直接交给模型；
/// 服务端不需要、也不该再持有这份中间解释。
struct DOODLE_CORE_API lighting_intent_t {
  std::string weather_{};

  friend void to_json(nlohmann::json& j, const lighting_intent_t& p) { j["weather"] = p.weather_; }
  friend void from_json(const nlohmann::json& j, lighting_intent_t& p) {
    if (j.contains("weather")) j.at("weather").get_to(p.weather_);
  }
};

struct DOODLE_CORE_API palette_entry_t {
  vec3_t color_{};
  interval_t share_{};

  friend void to_json(nlohmann::json& j, const palette_entry_t& p) {
    j["color"] = p.color_;
    j["share"] = p.share_;
  }
  friend void from_json(const nlohmann::json& j, palette_entry_t& p) {
    if (j.contains("color")) j.at("color").get_to(p.color_);
    if (j.contains("share")) j.at("share").get_to(p.share_);
  }
};

struct DOODLE_CORE_API luma_percentiles_t {
  interval_t p10_{};
  interval_t p50_{};
  interval_t p90_{};

  friend void to_json(nlohmann::json& j, const luma_percentiles_t& p) {
    j["p10"] = p.p10_;
    j["p50"] = p.p50_;
    j["p90"] = p.p90_;
  }
  friend void from_json(const nlohmann::json& j, luma_percentiles_t& p) {
    if (j.contains("p10")) j.at("p10").get_to(p.p10_);
    if (j.contains("p50")) j.at("p50").get_to(p.p50_);
    if (j.contains("p90")) j.at("p90").get_to(p.p90_);
  }
};

/// 高光/暗部裁剪率。区间而不是单值：参考是近似的。
struct DOODLE_CORE_API clip_rate_t {
  interval_t high_{0.0, 0.02};
  interval_t low_{0.0, 0.05};

  friend void to_json(nlohmann::json& j, const clip_rate_t& p) {
    j["high"] = p.high_;
    j["low"]  = p.low_;
  }
  friend void from_json(const nlohmann::json& j, clip_rate_t& p) {
    if (j.contains("high")) j.at("high").get_to(p.high_);
    if (j.contains("low")) j.at("low").get_to(p.low_);
  }
};

/// 调色倾向。注意：这里用可读标签而不是数值 —— 具体数值由 §9.2 全局调色反解去求。
struct DOODLE_CORE_API grading_hints_t {
  std::string vignette_{};     ///< none | subtle | moderate | strong
  bool lifted_blacks_{false};
  std::string bloom_{};        ///< none | subtle | moderate | strong

  friend void to_json(nlohmann::json& j, const grading_hints_t& p) {
    j["vignette"]      = p.vignette_;
    j["lifted_blacks"] = p.lifted_blacks_;
    j["bloom"]         = p.bloom_;
  }
  friend void from_json(const nlohmann::json& j, grading_hints_t& p) {
    if (j.contains("vignette")) j.at("vignette").get_to(p.vignette_);
    if (j.contains("lifted_blacks")) j.at("lifted_blacks").get_to(p.lifted_blacks_);
    if (j.contains("bloom")) j.at("bloom").get_to(p.bloom_);
  }
};

/// 责任归属：每个目标由谁负责达成。这一块是「不许改几何/材质」前提下
/// 把目标分派给灯光还是调色的依据。
struct DOODLE_CORE_API ownership_t {
  std::vector<std::string> lighting_{};
  std::vector<std::string> grading_{};
  std::vector<std::string> shared_{};

  friend void to_json(nlohmann::json& j, const ownership_t& p) {
    j["lighting"] = p.lighting_;
    j["grading"]  = p.grading_;
    j["shared"]   = p.shared_;
  }
  friend void from_json(const nlohmann::json& j, ownership_t& p) {
    if (j.contains("lighting")) j.at("lighting").get_to(p.lighting_);
    if (j.contains("grading")) j.at("grading").get_to(p.grading_);
    if (j.contains("shared")) j.at("shared").get_to(p.shared_);
  }
};

/// 目标：来自多张参考图的**显示空间**统计，聚合后。
/// space 恒为 display_srgb —— 指标在主指标空间（已 tonemap）里算，
/// 不是线性 EXR 空间；这一点是与早期设计的差别，见 §8.2。
struct DOODLE_CORE_API lighting_targets_t {
  std::string space_{"display_srgb"};
  luma_percentiles_t luma_percentiles_{};
  std::vector<palette_entry_t> palette_{};
  interval_t contrast_{0.55, 0.75};
  interval_t white_balance_kelvin_{3400.0, 4200.0};
  clip_rate_t clip_rate_{};
  grading_hints_t grading_hints_{};
  ownership_t ownership_{};

  friend void to_json(nlohmann::json& j, const lighting_targets_t& p) {
    j["space"]                = p.space_;
    j["luma_percentiles"]     = p.luma_percentiles_;
    j["palette"]              = p.palette_;
    j["contrast"]             = p.contrast_;
    j["white_balance_kelvin"] = p.white_balance_kelvin_;
    j["clip_rate"]            = p.clip_rate_;
    j["grading_hints"]        = p.grading_hints_;
    j["ownership"]            = p.ownership_;
  }
  friend void from_json(const nlohmann::json& j, lighting_targets_t& p) {
    if (j.contains("space")) j.at("space").get_to(p.space_);
    if (j.contains("luma_percentiles")) j.at("luma_percentiles").get_to(p.luma_percentiles_);
    if (j.contains("palette")) j.at("palette").get_to(p.palette_);
    if (j.contains("contrast")) j.at("contrast").get_to(p.contrast_);
    if (j.contains("white_balance_kelvin")) j.at("white_balance_kelvin").get_to(p.white_balance_kelvin_);
    if (j.contains("clip_rate")) j.at("clip_rate").get_to(p.clip_rate_);
    if (j.contains("grading_hints")) j.at("grading_hints").get_to(p.grading_hints_);
    if (j.contains("ownership")) j.at("ownership").get_to(p.ownership_);
  }
};

/// 多图聚合的过程与质量。冲突维度**不自动平均掉**，而是记进 conflicts 交人工裁决。
struct DOODLE_CORE_API reference_stats_t {
  std::int32_t image_count_{0};
  /// median | mean | trimmed_mean
  std::string aggregation_{"median"};
  std::map<std::string, double> per_dimension_confidence_{};
  /// 冲突维度名列表 → 前端提示美术裁决
  std::vector<std::string> conflicts_{};
  /// 检测到参考图被 AI/PS 修过（参考图本身就是修图产物，这里记的是「修图痕迹的强度」）
  bool retouch_detected_{false};

  friend void to_json(nlohmann::json& j, const reference_stats_t& p) {
    j["image_count"]                = p.image_count_;
    j["aggregation"]                = p.aggregation_;
    j["per_dimension_confidence"]   = p.per_dimension_confidence_;
    j["conflicts"]                  = p.conflicts_;
    j["retouch_detected"]           = p.retouch_detected_;
  }
  friend void from_json(const nlohmann::json& j, reference_stats_t& p) {
    if (j.contains("image_count")) j.at("image_count").get_to(p.image_count_);
    if (j.contains("aggregation")) j.at("aggregation").get_to(p.aggregation_);
    if (j.contains("per_dimension_confidence")) j.at("per_dimension_confidence").get_to(p.per_dimension_confidence_);
    if (j.contains("conflicts")) j.at("conflicts").get_to(p.conflicts_);
    if (j.contains("retouch_detected")) j.at("retouch_detected").get_to(p.retouch_detected_);
  }
};

/// Spec 的内存聚合形态（不是表实体）。感知层产出它，规划层与评判层消费它。
struct DOODLE_CORE_API lighting_spec_t {
  std::string schema_version_{std::string{g_spec_schema_version}};
  uuid spec_id_{};
  uuid session_id_{};
  lighting_intent_t intent_{};
  lighting_targets_t targets_{};
  reference_stats_t reference_stats_{};
  bool human_confirmed_{false};  ///< 人工闸门 1
  uuid confirmed_by_{};
  std::optional<chrono::system_zoned_time> confirmed_at_{};
  spec_degrade_level degrade_level_{spec_degrade_level::stats_only};

  friend void to_json(nlohmann::json& j, const lighting_spec_t& p) {
    j["schema_version"]  = p.schema_version_;
    j["spec_id"]         = p.spec_id_;
    j["session_id"]      = p.session_id_;
    j["intent"]          = p.intent_;
    j["targets"]         = p.targets_;
    j["reference_stats"] = p.reference_stats_;
    j["human_confirmed"] = p.human_confirmed_;
    j["confirmed_by"]    = p.confirmed_by_;
    j["confirmed_at"]    = p.confirmed_at_;
    j["degrade_level"]   = p.degrade_level_;
  }
  friend void from_json(const nlohmann::json& j, lighting_spec_t& p) {
    if (j.contains("schema_version")) j.at("schema_version").get_to(p.schema_version_);
    if (j.contains("spec_id")) j.at("spec_id").get_to(p.spec_id_);
    if (j.contains("session_id")) j.at("session_id").get_to(p.session_id_);
    if (j.contains("intent")) j.at("intent").get_to(p.intent_);
    if (j.contains("targets")) j.at("targets").get_to(p.targets_);
    if (j.contains("reference_stats")) j.at("reference_stats").get_to(p.reference_stats_);
    if (j.contains("human_confirmed")) j.at("human_confirmed").get_to(p.human_confirmed_);
    if (j.contains("confirmed_by")) j.at("confirmed_by").get_to(p.confirmed_by_);
    if (j.contains("confirmed_at")) j.at("confirmed_at").get_to(p.confirmed_at_);
    if (j.contains("degrade_level")) j.at("degrade_level").get_to(p.degrade_level_);
  }
};

}  // namespace doodle::ai_light
