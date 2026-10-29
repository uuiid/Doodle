#pragma once

#include <doodle_core/metadata/ai_light/iteration.h>

namespace doodle::ai_light {

struct DOODLE_CORE_API ai_light_current_iteration_t {
  std::int32_t iteration_{0};
  ai_light_iteration_state state_{ai_light_iteration_state::generated};
  std::optional<chrono::system_zoned_time> render_at_{};
  std::string feedback_text_{};
  nlohmann::json rigs_{};

  ai_light_current_iteration_t() = default;
  ai_light_current_iteration_t(const ai_light_iteration& in_iteration)
      : iteration_{in_iteration.iteration_},
        state_{in_iteration.state_},
        render_at_{in_iteration.render_at_},
        feedback_text_{in_iteration.feedback_text_},
        rigs_{in_iteration.rigs_} {}

  friend void to_json(nlohmann::json& j, const ai_light_current_iteration_t& p) {
    j["iteration"]     = p.iteration_;
    j["state"]         = p.state_;
    j["render_at"]     = p.render_at_;
    j["feedback_text"] = p.feedback_text_;
    j["rigs"]          = p.rigs_;
  }
  friend void from_json(const nlohmann::json& j, ai_light_current_iteration_t& p) {
    if (j.contains("iteration")) j.at("iteration").get_to(p.iteration_);
    if (j.contains("state")) j.at("state").get_to(p.state_);
    if (j.contains("render_at")) j.at("render_at").get_to(p.render_at_);
    if (j.contains("feedback_text")) j.at("feedback_text").get_to(p.feedback_text_);
    if (j.contains("rigs")) j.at("rigs").get_to(p.rigs_);
  }
};

}  // namespace doodle::ai_light
