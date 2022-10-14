#include "process_message.h"

#include <doodle_core/core/doodle_lib.h>
#include <doodle_core/lib_warp/boost_fmt_rational.h>
#include <doodle_core/lib_warp/json_warp.h>

#include <boost/numeric/conversion/cast.hpp>
#include <magic_enum.hpp>
#include <range/v3/all.hpp>
#include <range/v3/range.hpp>
#include <spdlog/spdlog.h>
namespace doodle {
void to_json(nlohmann::json& nlohmann_json_j, const process_message& nlohmann_json_t) {
  nlohmann_json_j["time"]     = nlohmann_json_t.p_time;
  nlohmann_json_j["end"]      = nlohmann_json_t.p_end;
  //  nlohmann_json_j["err"]      = nlohmann_json_t.p_err;
  //  nlohmann_json_j["log"]      = nlohmann_json_t.p_log;
  nlohmann_json_j["str_end"]  = nlohmann_json_t.p_str_end;
  nlohmann_json_j["name"]     = nlohmann_json_t.p_name;
  nlohmann_json_j["name_id"]  = nlohmann_json_t.p_name_id;
  nlohmann_json_j["state"]    = nlohmann_json_t.p_state;
  nlohmann_json_j["progress"] = nlohmann_json_t.p_progress;
}
void from_json(const nlohmann::json& nlohmann_json_j, process_message& nlohmann_json_t) {
  nlohmann_json_j.at("time").get_to(nlohmann_json_t.p_time);
  nlohmann_json_j.at("end").get_to(nlohmann_json_t.p_end);
  //  nlohmann_json_j.at("err").get_to(nlohmann_json_t.p_err);
  //  nlohmann_json_j.at("log").get_to(nlohmann_json_t.p_log);
  nlohmann_json_j.at("str_end").get_to(nlohmann_json_t.p_str_end);
  nlohmann_json_j.at("name").get_to(nlohmann_json_t.p_name);
  nlohmann_json_j.at("name_id").get_to(nlohmann_json_t.p_name_id);
  nlohmann_json_j.at("state").get_to(nlohmann_json_t.p_state);
  nlohmann_json_j.at("progress").get_to(nlohmann_json_t.p_progress);
}
process_message::process_message() : p_state(state::wait), p_time(chrono::system_clock::now()), p_name_id("##none") {}

const std::string& process_message::get_name() const {
  //  std::lock_guard _lock{_mutex};
  return p_name;
}
void process_message::set_name(const std::string& in_string) {
  std::lock_guard _lock{_mutex};
  p_name    = in_string;
  p_name_id = fmt::format("{}##{}", get_name(), fmt::ptr(this));
}
void process_message::progress_step(const rational_int& in_rational_int) {
  std::lock_guard _lock{_mutex};
  p_progress += in_rational_int;
}
void process_message::message(const std::string& in_string, const level& in_level_enum) {
  std::lock_guard _lock{_mutex};
  auto l_msg{in_string};
  //  l_msg |= ranges::actions::remove_if([](const std::string::value_type& in_type) -> bool {
  //    return in_type == '\n' || in_type == '\r';
  //  });
  if (l_msg.empty()) return;
  //  l_msg += '\n';

  spdlog::info(l_msg);
  switch (in_level_enum) {
    case level::warning:
      p_err += l_msg;
    default:
      p_log += l_msg;
      p_str_end = l_msg;
      break;
  }
}
void process_message::set_state(state in_state) {
  std::lock_guard _lock{_mutex};
  switch (in_state) {
    case run:
      p_time = chrono::system_clock::now();
    case wait:
      break;
    case success:
    case fail:
      p_end      = chrono::system_clock::now();
      p_progress = {1, 1};
      break;
  }
  p_state = in_state;
}
std::string_view process_message::err() const {
  //  std::lock_guard _lock{_mutex};
  return p_err;
}
std::string_view process_message::log() const {
  //  std::lock_guard _lock{_mutex};
  return p_log;
}
rational_int process_message::get_progress() const {
  //  std::lock_guard _lock{_mutex};
  return p_progress;
}
const process_message::state& process_message::get_state() const {
  //  std::lock_guard _lock{_mutex};
  return p_state;
}
chrono::sys_time_pos::duration process_message::get_time() const {
  //  std::lock_guard _lock{_mutex};
  return p_end ? (*p_end - p_time) : (chrono::system_clock::now() - p_time);
}
const std::string& process_message::message_back() const { return p_str_end; }

process_message::process_message(process_message&& in) noexcept {
  std::lock_guard _lock{_mutex};
  //  std::lock_guard _lock_in{in._mutex};
  p_time     = in.p_time;
  p_end      = in.p_end;
  p_err      = std::move(in.p_err);
  p_log      = std::move(in.p_log);
  p_name     = std::move(in.p_name);
  p_state    = in.p_state;
  p_progress = in.p_progress;
}
process_message& process_message::operator=(process_message&& in) noexcept {
  std::lock_guard _lock{_mutex};
  //  std::lock_guard _lock_in{in._mutex};
  p_time     = in.p_time;
  p_end      = in.p_end;
  p_err      = std::move(in.p_err);
  p_log      = std::move(in.p_log);
  p_name     = std::move(in.p_name);
  p_state    = in.p_state;
  p_progress = in.p_progress;
  return *this;
}
process_message::process_message(const process_message& in) noexcept {
  std::lock_guard _lock{_mutex};
  p_time     = in.p_time;
  p_end      = in.p_end;
  p_err      = in.p_err;
  p_log      = in.p_log;
  p_name     = in.p_name;
  p_state    = in.p_state;
  p_progress = in.p_progress;
}
process_message& process_message::operator=(const process_message& in) noexcept {
  std::lock_guard _lock{_mutex};
  p_time     = in.p_time;
  p_end      = in.p_end;
  p_err      = in.p_err;
  p_log      = in.p_log;
  p_name     = in.p_name;
  p_state    = in.p_state;
  p_progress = in.p_progress;
  return *this;
}
const std::string& process_message::get_name_id() const { return p_name_id; }
void process_message::progress_clear() {
  std::lock_guard _lock{_mutex};
  p_progress = 0;
}

}  // namespace doodle
