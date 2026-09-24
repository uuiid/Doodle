//
// AI 灯光：参考图（ai_light_reference）
//
// 参考图必有多张（同一场景的不同角度、经 AI/PS 修过、角色基本没有），
// 所以一张图一行。
//
// 为什么每张图单独存统计，而不是只存聚合结果：
//   方案 §5.3 要求冲突维度「不自动平均掉」并交人工裁决。聚合结果进 ai_light_spec，
//   但每张图的原始统计必须留在这里 —— 否则美术无法看到「到底是哪张图把色温拉偏了」，
//   裁决就无从下手。
//
// 这些图**不注册进 preview_file**：镜头预览列表只呈现交付结果（定稿候选），
// 参考图由 /api/doodle/ai/light/reference/{uuid}/image 直接从 file_path 提供。
//

#pragma once

#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/metadata/ai_light/common.h>
#include <doodle_core/metadata/ai_light/rig.h>
#include <doodle_core/metadata/base.h>

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace doodle::ai_light {

/// 单张图的亮度分位（标量，不是区间 —— 区间是「多张聚合」才有的东西）
struct DOODLE_CORE_API luma_percentiles_value_t {
  double p10_{0.0};
  double p50_{0.0};
  double p90_{0.0};

  friend void to_json(nlohmann::json& j, const luma_percentiles_value_t& p) {
    j["p10"] = p.p10_;
    j["p50"] = p.p50_;
    j["p90"] = p.p90_;
  }
  friend void from_json(const nlohmann::json& j, luma_percentiles_value_t& p) {
    if (j.contains("p10")) j.at("p10").get_to(p.p10_);
    if (j.contains("p50")) j.at("p50").get_to(p.p50_);
    if (j.contains("p90")) j.at("p90").get_to(p.p90_);
  }
};

struct DOODLE_CORE_API clip_rate_value_t {
  double high_{0.0};
  double low_{0.0};

  friend void to_json(nlohmann::json& j, const clip_rate_value_t& p) {
    j["high"] = p.high_;
    j["low"]  = p.low_;
  }
  friend void from_json(const nlohmann::json& j, clip_rate_value_t& p) {
    if (j.contains("high")) j.at("high").get_to(p.high_);
    if (j.contains("low")) j.at("low").get_to(p.low_);
  }
};

struct DOODLE_CORE_API image_palette_entry_t {
  vec3_t color_{};
  double share_{0.0};

  friend void to_json(nlohmann::json& j, const image_palette_entry_t& p) {
    j["color"] = p.color_;
    j["share"] = p.share_;
  }
  friend void from_json(const nlohmann::json& j, image_palette_entry_t& p) {
    if (j.contains("color")) j.at("color").get_to(p.color_);
    if (j.contains("share")) j.at("share").get_to(p.share_);
  }
};

/// 单张参考图在**显示空间**（已 tonemap）的统计。
/// 指标在主指标空间里算，不是线性 EXR 空间 —— 见方案 §8.2。
struct DOODLE_CORE_API reference_image_stats_t {
  std::string space_{"display_srgb"};
  luma_percentiles_value_t luma_percentiles_{};
  std::vector<image_palette_entry_t> palette_{};
  double contrast_{0.0};
  double white_balance_kelvin_{0.0};
  clip_rate_value_t clip_rate_{};
  /// 修图痕迹强度 0..1。参考图本身就是 AI/PS 修过的产物，
  /// 这个分数越高，越说明该图的统计不能当「物理真值」用（只能当氛围目标）。
  double retouch_score_{0.0};

  friend void to_json(nlohmann::json& j, const reference_image_stats_t& p) {
    j["space"]                = p.space_;
    j["luma_percentiles"]     = p.luma_percentiles_;
    j["palette"]              = p.palette_;
    j["contrast"]             = p.contrast_;
    j["white_balance_kelvin"] = p.white_balance_kelvin_;
    j["clip_rate"]            = p.clip_rate_;
    j["retouch_score"]        = p.retouch_score_;
  }
  friend void from_json(const nlohmann::json& j, reference_image_stats_t& p) {
    if (j.contains("space")) j.at("space").get_to(p.space_);
    if (j.contains("luma_percentiles")) j.at("luma_percentiles").get_to(p.luma_percentiles_);
    if (j.contains("palette")) j.at("palette").get_to(p.palette_);
    if (j.contains("contrast")) j.at("contrast").get_to(p.contrast_);
    if (j.contains("white_balance_kelvin")) j.at("white_balance_kelvin").get_to(p.white_balance_kelvin_);
    if (j.contains("clip_rate")) j.at("clip_rate").get_to(p.clip_rate_);
    if (j.contains("retouch_score")) j.at("retouch_score").get_to(p.retouch_score_);
  }
};

// ---------------------------------------------------------------------------
// 表实体：ai_light_reference
// ---------------------------------------------------------------------------

struct DOODLE_CORE_API ai_light_reference {
  DOODLE_BASE_FIELDS()

  uuid session_id_{};
  /// 0-based，前端按它排序
  std::int32_t ordinal_{0};

  /// 图片落盘路径。不注册 preview_file，由专用接口提供。
  FSys::path file_path_{};
  FSys::path thumbnail_path_{};

  std::int32_t width_{0};
  std::int32_t height_{0};

  /// reference_image_stats_t 的 JSON 形态。
  /// 嵌套结构不是 ORM 的列类型，所以以 nlohmann::json 落库，用 get_stats() 类型化取用。
  nlohmann::json stats_{};
  /// 该图统计的可信度 0..1（受修图痕迹、分辨率、构图影响）
  double stats_confidence_{0.0};
  /// 被外点剔除（例如构图里根本没有主体、或与其余图冲突过大）
  bool is_outlier_{false};
  std::string excluded_reason_{};

  chrono::system_zoned_time created_at_{chrono::current_zone(), chrono::system_clock::now()};

  reference_image_stats_t get_stats() const { return json_get_or_default<reference_image_stats_t>(stats_); }
  void set_stats(const reference_image_stats_t& in_stats) { stats_ = in_stats; }

  friend void to_json(nlohmann::json& j, const ai_light_reference& p) {
    j["id"]                = p.uuid_id_;
    j["session_id"]        = p.session_id_;
    j["ordinal"]           = p.ordinal_;
    j["file_path"]         = p.file_path_;
    j["thumbnail_path"]    = p.thumbnail_path_;
    j["width"]             = p.width_;
    j["height"]            = p.height_;
    j["stats"]             = p.stats_;
    j["stats_confidence"]  = p.stats_confidence_;
    j["is_outlier"]        = p.is_outlier_;
    j["excluded_reason"]   = p.excluded_reason_;
    j["created_at"]        = p.created_at_;
  }
  friend void from_json(const nlohmann::json& j, ai_light_reference& p) {
    if (j.contains("id")) j.at("id").get_to(p.uuid_id_);
    if (j.contains("session_id")) j.at("session_id").get_to(p.session_id_);
    if (j.contains("ordinal")) j.at("ordinal").get_to(p.ordinal_);
    if (j.contains("file_path")) j.at("file_path").get_to(p.file_path_);
    if (j.contains("thumbnail_path")) j.at("thumbnail_path").get_to(p.thumbnail_path_);
    if (j.contains("width")) j.at("width").get_to(p.width_);
    if (j.contains("height")) j.at("height").get_to(p.height_);
    if (j.contains("stats")) j.at("stats").get_to(p.stats_);
    if (j.contains("stats_confidence")) j.at("stats_confidence").get_to(p.stats_confidence_);
    if (j.contains("is_outlier")) j.at("is_outlier").get_to(p.is_outlier_);
    if (j.contains("excluded_reason")) j.at("excluded_reason").get_to(p.excluded_reason_);
  }
};

}  // namespace doodle::ai_light
