#include "progress_data.h"

namespace doodle {
void progress_data::update_progress(std::int32_t in_progress) {
  progress_rational_ += (in_progress * current_steps_) * total_steps_;
}

void progress_data::set_total_steps(std::int32_t in_total_steps) { total_steps_.assign(1, in_total_steps); }
std::int32_t progress_data::get_total_steps() const { return total_steps_.numerator(); }

void progress_data::set_current_steps(std::int32_t in_current_steps) { current_steps_.assign(1, in_current_steps); }
std::int32_t progress_data::get_current_steps() const { return current_steps_.numerator(); }

progress_data& progress_data::operator++() {
  update_progress(1);
  return *this;
}

}  // namespace doodle