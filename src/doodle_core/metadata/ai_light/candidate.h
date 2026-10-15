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

struct DOODLE_CORE_API vlm_delta_t {
  std::string model_{};
  std::string verdict_{};
  std::vector<std::string> differences_{};
  std::string raw_text_{};

  friend void to_json(nlohmann::json& j, const vlm_delta_t& p) {
    j["model"]       = p.model_;
    j["verdict"]     = p.verdict_;
    j["differences"] = p.differences_;
    j["raw_text"]    = p.raw_text_;
  }
  friend void from_json(const nlohmann::json& j, vlm_delta_t& p) {
    if (j.contains("model")) j.at("model").get_to(p.model_);
    if (j.contains("verdict")) j.at("verdict").get_to(p.verdict_);
    if (j.contains("differences")) j.at("differences").get_to(p.differences_);
    if (j.contains("raw_text")) j.at("raw_text").get_to(p.raw_text_);
  }
};

struct DOODLE_CORE_API ai_light_candidate {
  DOODLE_BASE_FIELDS()

  uuid iteration_id_{};
  uuid session_id_{};

  std::int32_t candidate_index_{0};

  nlohmann::json rig_{};

  candidate_render_status render_status_{candidate_render_status::pending};
  std::int64_t render_ms_{0};
  std::int32_t light_count_{0};

  nlohmann::json metrics_{};

  std::optional<double> vlm_score_{};
  nlohmann::json vlm_delta_{};

  bool is_selected_{false};
  bool is_accepted_{false};
  uuid accepted_session_id_{};

  uuid preview_file_id_{};

  chrono::system_zoned_time created_at_{chrono::current_zone(), chrono::system_clock::now()};

  lighting_rig_t get_rig() const { return json_get_or_default<lighting_rig_t>(rig_); }
  void set_rig(const lighting_rig_t& in_rig) { rig_ = in_rig; }
  candidate_metrics_t get_metrics() const { return json_get_or_default<candidate_metrics_t>(metrics_); }
  void set_metrics(const candidate_metrics_t& in_metrics) { metrics_ = in_metrics; }
  vlm_delta_t get_vlm_delta() const { return json_get_or_default<vlm_delta_t>(vlm_delta_); }
  void set_vlm_delta(const vlm_delta_t& in_delta) { vlm_delta_ = in_delta; }

  void sync_from_rig() {
    if (rig_.is_null()) return;
    const auto l_rig = get_rig();
    candidate_index_ = l_rig.candidate_index_;
    session_id_      = l_rig.session_id_;
  }
  void sync_to_rig() {
    auto l_rig             = get_rig();
    l_rig.candidate_index_ = candidate_index_;
    l_rig.session_id_      = session_id_;
    set_rig(l_rig);
  }

  void mark_accepted() {
    is_accepted_         = true;
    accepted_session_id_ = session_id_;
  }
  void clear_accepted() {
    is_accepted_         = false;
    accepted_session_id_ = uuid{};
  }
  bool is_accepted_candidate() const { return !accepted_session_id_.is_nil() && accepted_session_id_ == session_id_; }

  friend void to_json(nlohmann::json& j, const ai_light_candidate& p) {
    j["id"]                  = p.uuid_id_;
    j["iteration_id"]        = p.iteration_id_;
    j["session_id"]          = p.session_id_;
    j["candidate_index"]     = p.candidate_index_;
    j["rig"]                 = p.rig_;
    j["render_status"]       = p.render_status_;
    j["render_ms"]           = p.render_ms_;
    j["light_count"]         = p.light_count_;
    j["metrics"]             = p.metrics_;
    j["vlm_score"]           = p.vlm_score_;
    j["vlm_delta"]           = p.vlm_delta_;
    j["is_selected"]         = p.is_selected_;
    j["is_accepted"]         = p.is_accepted_;
    j["accepted_session_id"] = p.accepted_session_id_;
    j["preview_file_id"]     = p.preview_file_id_;
    j["created_at"]          = p.created_at_;
  }
  friend void from_json(const nlohmann::json& j, ai_light_candidate& p) {
    if (j.contains("id")) j.at("id").get_to(p.uuid_id_);
    if (j.contains("iteration_id")) j.at("iteration_id").get_to(p.iteration_id_);
    if (j.contains("session_id")) j.at("session_id").get_to(p.session_id_);
    if (j.contains("candidate_index")) j.at("candidate_index").get_to(p.candidate_index_);
    if (j.contains("rig")) j.at("rig").get_to(p.rig_);
    if (j.contains("render_status")) j.at("render_status").get_to(p.render_status_);
    if (j.contains("render_ms")) j.at("render_ms").get_to(p.render_ms_);
    if (j.contains("light_count")) j.at("light_count").get_to(p.light_count_);
    if (j.contains("metrics")) j.at("metrics").get_to(p.metrics_);
    if (j.contains("vlm_score")) j.at("vlm_score").get_to(p.vlm_score_);
    if (j.contains("vlm_delta")) j.at("vlm_delta").get_to(p.vlm_delta_);
    if (j.contains("is_selected")) j.at("is_selected").get_to(p.is_selected_);
    if (j.contains("is_accepted")) j.at("is_accepted").get_to(p.is_accepted_);
    if (j.contains("accepted_session_id")) j.at("accepted_session_id").get_to(p.accepted_session_id_);
    if (j.contains("preview_file_id")) j.at("preview_file_id").get_to(p.preview_file_id_);
  }
};

}  // namespace doodle::ai_light
