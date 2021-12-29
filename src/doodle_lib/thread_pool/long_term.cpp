#include "long_term.h"

#include <doodle_lib/core/doodle_lib.h>

#include <boost/numeric/conversion/cast.hpp>
#include <magic_enum.hpp>
namespace doodle {
long_term::long_term() : sig_progress(),
                         sig_message_result(),
                         sig_finished(),
                         p_fulfil(false),
                         p_str(),
                         p_log(),
                         p_progress(0),
                         _mutex(),
                         p_list(),
                         p_child(),
                         p_id(),
                         p_name(),
                         p_time(chrono::system_clock::now()),
                         p_end(),
                         p_state(state::wait) {
  sig_finished.connect([this]() {
    std::lock_guard k_guard{_mutex};
    p_fulfil   = true;
    p_progress = 1;
    p_end      = chrono::system_clock::now();
    p_state    = success;
  });
  sig_message_result.connect(
      [this](const std::string& in_str, level in_level) {
        {
          std::lock_guard k_guard{_mutex};
          switch (in_level) {
            case warning: {
              p_str.push_back(in_str);
            }
            case info: {
              p_log.push_back(in_str);
            } break;
            default:
              break;
          }
        }
        DOODLE_LOG_INFO(in_str);
      });
  sig_progress.connect([this](rational_int in) {
    step(in);
  });

  p_id = fmt::format("none###{}", fmt::ptr(this));
}

rational_int long_term::step(rational_int in_) {
  std::lock_guard k_guard{_mutex};
  p_progress += in_;
  return p_progress;
}

bool long_term::fulfil() const {
  return p_fulfil;
}

std::string long_term::message_result() const {
  return p_str.empty() ? std::string{} : p_str.back();
}

long_term::~long_term() {
  for (auto& k_item : p_list) {
    try {
      if (k_item.valid())
        k_item.get();
    } catch (const doodle_error& error) {
      DOODLE_LOG_WARN(error.what());
    }
  }
}
void long_term::post_constructor() {
  auto& k_ = doodle_lib::Get();
  {
    std::lock_guard k_guard{k_.mutex};
    k_.long_task_list.push_back(shared_from_this());
  }
}
std::string& long_term::get_id() {
  return p_id;
}
std::string& long_term::get_name() {
  return p_name;
}
void long_term::set_name(const std::string& in_string) {
  p_name = in_string;
  p_id   = fmt::format("{}##{}", in_string, fmt::ptr(this));
}
rational_int long_term::get_progress() const {
  return p_progress;
}
std::double_t long_term::get_progress_int() const {
  return boost::rational_cast<std::double_t>(p_progress * rational_int{100});
}
std::string_view long_term::get_state_str() const {
  return magic_enum::enum_name(p_state);
}
std::string long_term::get_time_str() const {
  if (p_state == wait || p_state == none_)
    return {"..."};

  if (p_end) {
    return chrono::format("%H:%M:%S",
                          chrono::floor<chrono::seconds>(*p_end - p_time));
  } else {
    return chrono::format("%H:%M:%S",
                          chrono::floor<chrono::seconds>(chrono::system_clock::now() - p_time));
  }
}
const std::deque<std::string>& long_term::message() const {
  return p_str;
}
const std::deque<std::string>& long_term::log() const {
  return p_log;
}
void long_term::start() {
  p_time  = chrono::system_clock::now();
  p_state = run;
}
void long_term::set_state(long_term::state in_state) {
  p_state = in_state;
}

process_message::process_message()
    : p_state(state::wait),
      p_time(chrono::system_clock::now()) {
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
