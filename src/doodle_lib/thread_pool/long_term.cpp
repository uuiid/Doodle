#include "long_term.h"

#include <doodle_lib/core/doodle_lib.h>

#include <boost/numeric/conversion/cast.hpp>
#include <magic_enum.hpp>
namespace doodle {

process_message::process_message()
    : p_state(state::wait),
      p_time(chrono::system_clock::now()),
      p_name_id("##none") {
}

const std::string& process_message::get_name() const {
  //  std::lock_guard _lock{_mutex};
  return p_name;
}
void process_message::set_name(const string& in_string) {
  std::lock_guard _lock{_mutex};
  p_name    = in_string;
  p_name_id = fmt::format("{}##{}", get_name(), fmt::ptr(this));
}
void process_message::progress_step(const rational_int& in_rational_int) {
  std::lock_guard _lock{_mutex};
  p_progress += in_rational_int;
}
void process_message::message(const string& in_string, const level& in_level_enum) {
  spdlog::info(in_string);
  std::lock_guard _lock{_mutex};
  switch (in_level_enum) {
    case level::warning:
      p_err += in_string;
    default:
      p_log += in_string;
      p_str_end = in_string;
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
const std::string& process_message::message_back() const {
  return p_str_end;
}

process_message::process_message(process_message&& in) noexcept {
  p_time     = in.p_time;
  p_end      = in.p_end;
  p_err      = std::move(in.p_err);
  p_log      = std::move(in.p_log);
  p_name     = std::move(in.p_name);
  p_state    = in.p_state;
  p_progress = in.p_progress;
}
process_message& process_message::operator=(process_message&& in) noexcept {
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
  p_time     = in.p_time;
  p_end      = in.p_end;
  p_err      = in.p_err;
  p_log      = in.p_log;
  p_name     = in.p_name;
  p_state    = in.p_state;
  p_progress = in.p_progress;
}
process_message& process_message::operator=(const process_message& in) noexcept {
  p_time     = in.p_time;
  p_end      = in.p_end;
  p_err      = in.p_err;
  p_log      = in.p_log;
  p_name     = in.p_name;
  p_state    = in.p_state;
  p_progress = in.p_progress;
  return *this;
}
const std::string& process_message::get_name_id() const {
  return p_name_id;
}

}  // namespace doodle
