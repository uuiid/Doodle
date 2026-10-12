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
/// 所以本实现只保留 degrade_level 这一个字段，Spec schema 记为 1.3。
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

/// 角色性格 → 灯光语言（docs/ue_ai_lighting_plan.md §5.5）。
/// 这是「人物角色性格」这类人工提示词进入灯光参数的唯一通道：先落成可读的灯光语言，
/// 再由规划层翻译成具体灯位与参数。
struct DOODLE_CORE_API lighting_language_t {
  std::optional<interval_t> key_fill_ratio_{};
  std::string hardness_{};       ///< hard | soft | very_soft
  std::string position_bias_{};  ///< side_high | front_low | back_high ...
  std::string kelvin_bias_{};    ///< cool | warm | neutral

  friend void to_json(nlohmann::json& j, const lighting_language_t& p) {
    if (p.key_fill_ratio_) j["key_fill_ratio"] = *p.key_fill_ratio_;
    if (!p.hardness_.empty()) j["hardness"] = p.hardness_;
    if (!p.position_bias_.empty()) j["position_bias"] = p.position_bias_;
    if (!p.kelvin_bias_.empty()) j["kelvin_bias"] = p.kelvin_bias_;
  }
  friend void from_json(const nlohmann::json& j, lighting_language_t& p) {
    if (j.contains("key_fill_ratio") && !j.at("key_fill_ratio").is_null())
      j.at("key_fill_ratio").get_to(p.key_fill_ratio_.emplace());
    if (j.contains("hardness")) j.at("hardness").get_to(p.hardness_);
    if (j.contains("position_bias")) j.at("position_bias").get_to(p.position_bias_);
    if (j.contains("kelvin_bias")) j.at("kelvin_bias").get_to(p.kelvin_bias_);
  }
};

struct DOODLE_CORE_API character_personality_t {
  std::string subject_{};  ///< 主体语义 id
  std::string trait_{};    ///< 性格描述，如「阴郁内向」
  lighting_language_t lighting_language_{};

  friend void to_json(nlohmann::json& j, const character_personality_t& p) {
    j["subject"]           = p.subject_;
    j["trait"]             = p.trait_;
    j["lighting_language"] = p.lighting_language_;
  }
  friend void from_json(const nlohmann::json& j, character_personality_t& p) {
    if (j.contains("subject")) j.at("subject").get_to(p.subject_);
    if (j.contains("trait")) j.at("trait").get_to(p.trait_);
    if (j.contains("lighting_language")) j.at("lighting_language").get_to(p.lighting_language_);
  }
};

/// 意图：来自人工提示词，**权威**。与参考统计冲突时以本块为准（§6.3）。
struct DOODLE_CORE_API lighting_intent_t {
  std::vector<std::string> atmosphere_{};
  std::string time_of_day_{};
  std::string weather_{};
  std::vector<std::string> source_motivation_{};
  std::vector<character_personality_t> character_personality_{};
  std::string story_context_{};
  std::string setting_{};

  friend void to_json(nlohmann::json& j, const lighting_intent_t& p) {
    j["atmosphere"]             = p.atmosphere_;
    j["time_of_day"]            = p.time_of_day_;
    j["weather"]                = p.weather_;
    j["source_motivation"]      = p.source_motivation_;
    j["character_personality"]  = p.character_personality_;
    j["story_context"]          = p.story_context_;
    j["setting"]                = p.setting_;
  }
  friend void from_json(const nlohmann::json& j, lighting_intent_t& p) {
    if (j.contains("atmosphere")) j.at("atmosphere").get_to(p.atmosphere_);
    if (j.contains("time_of_day")) j.at("time_of_day").get_to(p.time_of_day_);
    if (j.contains("weather")) j.at("weather").get_to(p.weather_);
    if (j.contains("source_motivation")) j.at("source_motivation").get_to(p.source_motivation_);
    if (j.contains("character_personality")) j.at("character_personality").get_to(p.character_personality_);
    if (j.contains("story_context")) j.at("story_context").get_to(p.story_context_);
    if (j.contains("setting")) j.at("setting").get_to(p.setting_);
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
  /// 恒为 false：本工具不做物理反解。保留字段是为了让工件自描述，
  /// 避免下游误以为可以做物理反解。
  bool usable_for_physical_solve_{false};

  friend void to_json(nlohmann::json& j, const reference_stats_t& p) {
    j["image_count"]                = p.image_count_;
    j["aggregation"]                = p.aggregation_;
    j["per_dimension_confidence"]   = p.per_dimension_confidence_;
    j["conflicts"]                  = p.conflicts_;
    j["retouch_detected"]           = p.retouch_detected_;
    j["usable_for_physical_solve"]  = p.usable_for_physical_solve_;
  }
  friend void from_json(const nlohmann::json& j, reference_stats_t& p) {
    if (j.contains("image_count")) j.at("image_count").get_to(p.image_count_);
    if (j.contains("aggregation")) j.at("aggregation").get_to(p.aggregation_);
    if (j.contains("per_dimension_confidence")) j.at("per_dimension_confidence").get_to(p.per_dimension_confidence_);
    if (j.contains("conflicts")) j.at("conflicts").get_to(p.conflicts_);
    if (j.contains("retouch_detected")) j.at("retouch_detected").get_to(p.retouch_detected_);
    // usable_for_physical_solve 是常量 false，不从 JSON 读入，避免下游把它改成 true。
    p.usable_for_physical_solve_ = false;
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

/// 冲突维度的裁决结果（人工闸门 1 的一部分）
struct DOODLE_CORE_API conflict_resolution_t {
  /// 冲突维度名 → 被采纳的值（json 是为了容纳区间/调色板等不同形状）
  std::map<std::string, nlohmann::json> resolved_{};
  std::string note_{};

  friend void to_json(nlohmann::json& j, const conflict_resolution_t& p) {
    j["resolved"] = p.resolved_;
    if (!p.note_.empty()) j["note"] = p.note_;
  }
  friend void from_json(const nlohmann::json& j, conflict_resolution_t& p) {
    if (j.contains("resolved")) j.at("resolved").get_to(p.resolved_);
    if (j.contains("note")) j.at("note").get_to(p.note_);
  }
};

// ---------------------------------------------------------------------------
// 表实体：ai_light_spec
// ---------------------------------------------------------------------------

/// 会话内版本化的灯光目标。重读参考会产生新的 revision。
///
/// created_at_ / updated_at_ 的**名字与类型必须一字不差** —— ORM 靠名字探测并自动填充
/// （见 sqlite_orm/orm/insert.h:95-98）。
///
/// intent / targets / reference_stats / conflict_resolution 都是 nlohmann::json：
/// 它们是嵌套结构，而 ORM 的列类型只覆盖标量/string/uuid/时间/枚举/vector<string>/json。
/// 类型化取用走下面那组访问器。
struct DOODLE_CORE_API ai_light_spec {
  DOODLE_BASE_FIELDS()

  uuid session_id_{};
  /// 会话内递增。权威版本号；「当前 Spec」= 会话内 revision 最大的一行。
  std::int32_t revision_{1};
  std::string schema_version_{std::string{g_spec_schema_version}};

  nlohmann::json intent_{};
  nlohmann::json targets_{};
  nlohmann::json reference_stats_{};
  spec_degrade_level degrade_level_{spec_degrade_level::stats_only};
  // 是否已经确认
  bool human_confirmed_{false};
  // 确认人和时间
  uuid confirmed_by_{};
  std::optional<chrono::system_zoned_time> confirmed_at_{};
  nlohmann::json conflict_resolution_{};

  chrono::system_zoned_time created_at_{chrono::current_zone(), chrono::system_clock::now()};
  chrono::system_zoned_time updated_at_{chrono::current_zone(), chrono::system_clock::now()};

  // --- 类型化访问器 ---
  lighting_intent_t get_intent() const { return json_get_or_default<lighting_intent_t>(intent_); }
  void set_intent(const lighting_intent_t& in_intent) { intent_ = in_intent; }
  lighting_targets_t get_targets() const { return json_get_or_default<lighting_targets_t>(targets_); }
  void set_targets(const lighting_targets_t& in_targets) { targets_ = in_targets; }
  reference_stats_t get_reference_stats() const {
    return json_get_or_default<reference_stats_t>(reference_stats_);
  }
  void set_reference_stats(const reference_stats_t& in_stats) { reference_stats_ = in_stats; }
  conflict_resolution_t get_conflict_resolution() const {
    return json_get_or_default<conflict_resolution_t>(conflict_resolution_);
  }
  void set_conflict_resolution(const conflict_resolution_t& in_resolution) {
    conflict_resolution_ = in_resolution;
  }

  /// 拼出完整的 §4.2 工件形态（表实体 → 内存聚合体）
  lighting_spec_t to_spec() const {
    lighting_spec_t l_ret{};
    l_ret.schema_version_  = schema_version_;
    l_ret.spec_id_         = uuid_id_;
    l_ret.session_id_      = session_id_;
    l_ret.intent_          = get_intent();
    l_ret.targets_         = get_targets();
    l_ret.reference_stats_ = get_reference_stats();
    l_ret.human_confirmed_ = human_confirmed_;
    l_ret.confirmed_by_    = confirmed_by_;
    l_ret.confirmed_at_    = confirmed_at_;
    l_ret.degrade_level_   = degrade_level_;
    return l_ret;
  }
  /// 从内存聚合体回填（感知层产出 lighting_spec_t 之后调用）
  void from_spec(const lighting_spec_t& in_spec) {
    schema_version_ = in_spec.schema_version_;
    session_id_     = in_spec.session_id_;
    set_intent(in_spec.intent_);
    set_targets(in_spec.targets_);
    set_reference_stats(in_spec.reference_stats_);
    human_confirmed_ = in_spec.human_confirmed_;
    confirmed_by_    = in_spec.confirmed_by_;
    confirmed_at_    = in_spec.confirmed_at_;
    degrade_level_   = in_spec.degrade_level_;
  }

  friend void to_json(nlohmann::json& j, const ai_light_spec& p) {
    j = p.to_spec();
    j["id"]                  = p.uuid_id_;
    j["revision"]            = p.revision_;
    j["conflict_resolution"] = p.conflict_resolution_;
    j["created_at"]          = p.created_at_;
    j["updated_at"]          = p.updated_at_;
  }
  friend void from_json(const nlohmann::json& j, ai_light_spec& p) {
    lighting_spec_t l_spec{};
    if (j.contains("intent") || j.contains("targets") || j.contains("reference_stats"))
      l_spec = j.get<lighting_spec_t>();
    p.from_spec(l_spec);
    if (j.contains("id")) j.at("id").get_to(p.uuid_id_);
    if (j.contains("revision")) j.at("revision").get_to(p.revision_);
    if (j.contains("conflict_resolution")) j.at("conflict_resolution").get_to(p.conflict_resolution_);
  }
};

}  // namespace doodle::ai_light
