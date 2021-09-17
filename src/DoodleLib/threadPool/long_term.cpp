#include "long_term.h"

#include <DoodleLib/core/DoodleLib.h>

#include <boost/numeric/conversion/cast.hpp>

namespace doodle {
long_term::long_term() : sig_progress(),
                         sig_message_result(),
                         sig_finished(),
                         p_fulfil(false),
                         p_str(),
                         p_progress(0),
                         _mutex(),
                         p_list(),
                         p_child(),
                         p_id(),
                         p_name(),
                         p_time(chrono::system_clock::now()),
                         p_state(state::none_) {
  sig_finished.connect([this]() {
    std::lock_guard k_guard{_mutex};
    p_fulfil = true;
  });
  sig_message_result.connect([this](const std::string& in_str) {
    if (p_fulfil)
      p_str = in_str;
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
  return p_str;
}
void long_term::forward_sig(const long_term_ptr& in_forward) {
  p_child.push_back(in_forward);
  in_forward->sig_progress.connect([this](rational_int in_pro) { sig_progress(in_pro); });
  in_forward->sig_finished.connect([this]() { sig_finished(); });
  in_forward->sig_message_result.connect([this](const std::string& in_str) { sig_message_result(in_str); });
}

void long_term::forward_sig(const std::vector<long_term_ptr>& in_forward) {
  const std::size_t k_size = in_forward.size();
  for (const auto& i : in_forward) {
    p_child.push_back(i);

    i->sig_progress.connect([this, k_size](
                                rational_int in_) {
      sig_progress(in_ / k_size);
    });
    i->sig_message_result.connect([this](const std::string& str) {
      sig_message_result(str);
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
      k_item.get();

    } catch (const DoodleError& error) {
      DOODLE_LOG_WARN(error.what());
    }
  }
}
long_term_ptr long_term::make_this_shared() {
  auto k_ptr = std::make_shared<long_term>();
  auto& k_   = DoodleLib::Get();
  {
    std::lock_guard k_guard{k_.mutex};
    k_.long_task_list.push_back(k_ptr);
  }
  return k_ptr;
}
std::string& long_term::get_id() {
  return p_id;
}
std::string& long_term::get_name() {
  return p_name;
}
void long_term::set_name(const std::string& in_string) {
  p_name = in_string;
  p_id   = fmt::format("{}###{}", in_string, fmt::ptr(this));
}
rational_int long_term::get_progress() const {
  return p_progress;
}
std::double_t long_term::get_progress_int() const {
  return boost::rational_cast<std::double_t>(p_progress * rational_int{100});
}
}  // namespace doodle
