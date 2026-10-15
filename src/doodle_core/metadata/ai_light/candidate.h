#pragma once

#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/metadata/ai_light/common.h>
#include <doodle_core/metadata/ai_light/reference.h>
#include <doodle_core/metadata/ai_light/rig.h>
#include <doodle_core/metadata/base.h>

#include <cstdint>
#include <filesystem>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace doodle::ai_light {

enum class candidate_render_status {
  pending,
  rendered,
  failed,
};
DOODLE_JSON_SERIALIZE_ENUM_STRICT(
    candidate_render_status, {
                                 {candidate_render_status::pending, "pending"},
                                 {candidate_render_status::rendered, "rendered"},
                                 {candidate_render_status::failed, "failed"},
                             }
)

struct DOODLE_CORE_API metric_landing_t {
  std::string dimension_{};
  double achieved_{0.0};
  interval_t target_{};
  bool in_interval_{false};
  double distance_{0.0};

  friend void to_json(nlohmann::json& j, const metric_landing_t& p) {
    j["dimension"]   = p.dimension_;
    j["achieved"]    = p.achieved_;
    j["target"]      = p.target_;
    j["in_interval"] = p.in_interval_;
    j["distance"]    = p.distance_;
  }
  friend void from_json(const nlohmann::json& j, metric_landing_t& p) {
    if (j.contains("dimension")) j.at("dimension").get_to(p.dimension_);
    if (j.contains("achieved")) j.at("achieved").get_to(p.achieved_);
    if (j.contains("target")) j.at("target").get_to(p.target_);
    if (j.contains("in_interval")) j.at("in_interval").get_to(p.in_interval_);
    if (j.contains("distance")) j.at("distance").get_to(p.distance_);
  }
};

struct DOODLE_CORE_API candidate_metrics_t {
  std::string space_{"display_srgb"};
  luma_percentiles_value_t luma_percentiles_{};
  std::vector<image_palette_entry_t> palette_{};
  double contrast_{0.0};
  double white_balance_kelvin_{0.0};
  clip_rate_value_t clip_rate_{};

  std::vector<metric_landing_t> landings_{};
  double satisfaction_{0.0};
  std::map<std::string, double> dimension_distances_{};

  friend void to_json(nlohmann::json& j, const candidate_metrics_t& p) {
    j["space"]                = p.space_;
    j["luma_percentiles"]     = p.luma_percentiles_;
    j["palette"]              = p.palette_;
    j["contrast"]             = p.contrast_;
    j["white_balance_kelvin"] = p.white_balance_kelvin_;
    j["clip_rate"]            = p.clip_rate_;
    j["landings"]             = p.landings_;
    j["satisfaction"]         = p.satisfaction_;
    j["dimension_distances"]  = p.dimension_distances_;
  }
  friend void from_json(const nlohmann::json& j, candidate_metrics_t& p) {
    if (j.contains("space")) j.at("space").get_to(p.space_);
    if (j.contains("luma_percentiles")) j.at("luma_percentiles").get_to(p.luma_percentiles_);
    if (j.contains("palette")) j.at("palette").get_to(p.palette_);
    if (j.contains("contrast")) j.at("contrast").get_to(p.contrast_);
    if (j.contains("white_balance_kelvin")) j.at("white_balance_kelvin").get_to(p.white_balance_kelvin_);
    if (j.contains("clip_rate")) j.at("clip_rate").get_to(p.clip_rate_);
    if (j.contains("landings")) j.at("landings").get_to(p.landings_);
    if (j.contains("satisfaction")) j.at("satisfaction").get_to(p.satisfaction_);
    if (j.contains("dimension_distances")) j.at("dimension_distances").get_to(p.dimension_distances_);
  }
};

// 表实体：ai_light_candidate
// 灯光候选
struct DOODLE_CORE_API ai_light_candidate {
  DOODLE_BASE_FIELDS()

  uuid iteration_id_{};
  uuid session_id_{};

  std::int32_t candidate_index_{0};

  nlohmann::json rig_{};

  candidate_render_status render_status_{candidate_render_status::pending};

  nlohmann::json metrics_{};

  bool is_selected_{false};

  uuid preview_file_id_{};

  chrono::system_zoned_time created_at_{chrono::current_zone(), chrono::system_clock::now()};

  lighting_rig_t get_rig() const { return rig_.get<lighting_rig_t>(); }
  void set_rig(const lighting_rig_t& in_rig) { rig_ = in_rig; }
  candidate_metrics_t get_metrics() const { return metrics_.get<candidate_metrics_t>(); }
  void set_metrics(const candidate_metrics_t& in_metrics) { metrics_ = in_metrics; }

  friend void to_json(nlohmann::json& j, const ai_light_candidate& p) {
    j["id"]              = p.uuid_id_;
    j["iteration_id"]    = p.iteration_id_;
    j["session_id"]      = p.session_id_;
    j["candidate_index"] = p.candidate_index_;
    j["rig"]             = p.rig_;
    j["render_status"]   = p.render_status_;
    j["metrics"]         = p.metrics_;
    j["is_selected"]     = p.is_selected_;
    j["preview_file_id"] = p.preview_file_id_;
    j["created_at"]      = p.created_at_;
  }
  friend void from_json(const nlohmann::json& j, ai_light_candidate& p) {
    if (j.contains("id")) j.at("id").get_to(p.uuid_id_);
    if (j.contains("iteration_id")) j.at("iteration_id").get_to(p.iteration_id_);
    if (j.contains("session_id")) j.at("session_id").get_to(p.session_id_);
    if (j.contains("candidate_index")) j.at("candidate_index").get_to(p.candidate_index_);
    if (j.contains("rig")) j.at("rig").get_to(p.rig_);
    if (j.contains("render_status")) j.at("render_status").get_to(p.render_status_);
    if (j.contains("metrics")) j.at("metrics").get_to(p.metrics_);
    if (j.contains("is_selected")) j.at("is_selected").get_to(p.is_selected_);
    if (j.contains("preview_file_id")) j.at("preview_file_id").get_to(p.preview_file_id_);
  }
};

}  // namespace doodle::ai_light
