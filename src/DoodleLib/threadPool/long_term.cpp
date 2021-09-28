#include "long_term.h"

#include <DoodleLib/core/doodle_lib.h>

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
void long_term::forward_sig(const long_term_ptr& in_forward) {
  p_child.push_back(in_forward);
  in_forward->sig_progress.connect([this](rational_int in_pro) { sig_progress(in_pro); });
  in_forward->sig_finished.connect([this]() { sig_finished(); });
  in_forward->sig_message_result.connect(
      [this](const std::string& in_str, level in_level) {
        sig_message_result(in_str, in_level);
      });
}

void long_term::forward_sig(const std::vector<long_term_ptr>& in_forward) {
  const std::size_t k_size = in_forward.size();
  for (const auto& i : in_forward) {
    p_child.push_back(i);

    i->sig_progress.connect([this, k_size](
                                rational_int in_) {
      sig_progress(in_ / k_size);
    });
    i->sig_message_result.connect([this](const std::string& str, level in_level) {
      sig_message_result(str, in_level);
    });
    i->sig_finished.connect([this, k_size]() {
      auto k_is_fulfil = std::all_of(p_child.begin(), p_child.end(), [](const long_term_ptr& in_term) {
        std::lock_guard k_guark{in_term->_mutex};
        return in_term->p_fulfil;
      });
      if (k_is_fulfil)
        sig_finished();
    });
  }
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
const std::deque<std::string> long_term::message() const {
  return p_str;
}
const std::deque<std::string> long_term::log() const {
  return p_log;
}
void long_term::start() {
  p_time  = chrono::system_clock::now();
  p_state = run;
}
void long_term::set_state(long_term::state in_state) {
  p_state = in_state;
}
}  // namespace doodle
