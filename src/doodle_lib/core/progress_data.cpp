#include "progress_data.h"

#include <doodle_lib/core/socket_io/broadcast.h>
#include <doodle_lib/lib_warp/boost_fmt_rational.h>

#include <boost/rational.hpp>

namespace doodle {

namespace {
struct progress_update_broadcast_t {
  const std::string event_name_;
  const std::string namespace_;
  uuid& id_;
  explicit progress_update_broadcast_t(
      std::string_view in_namespace, std::string_view in_event_name, double in_progress, uuid& in_id
  )
      : event_name_(in_event_name), namespace_(in_namespace), id_(in_id), progress_(in_progress) {}

  double progress_;
  // to json
  friend void to_json(nlohmann::json& j, const progress_update_broadcast_t& p) {
    j["progress"] = p.progress_;
    j["id"]       = p.id_;
  }
};
}  // namespace

void progress_data::update_progress(std::int32_t in_progress) {
  progress_rational_ += (in_progress * current_steps_) * total_steps_;
  constexpr reational_t one_hundred_percent{1, 100};
  if (progress_rational_ - last_emitted_progress_ >= one_hundred_percent) {
    // 发送事件
    // SPDLOG_INFO("Progress update: {}", progress_rational_);
    progress_update_broadcast_t broadcast{
        namespace_, event_name_, boost::rational_cast<std::double_t>(progress_rational_), uuid_id_
    };
    socket_io::broadcast(broadcast);
    while (progress_rational_ - last_emitted_progress_ >= one_hundred_percent)
      last_emitted_progress_ += one_hundred_percent;
  }
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