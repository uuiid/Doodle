#include "long_term.h"

#include <boost/numeric/conversion/cast.hpp>
namespace doodle {
long_term::long_term() : sig_progress(),
                         sig_message_result(),
                         sig_finished(),
                         p_fulfil(false),
                         p_str(),
                         p_progress(0),
                         p_num(0),
                         _mutex(),
                         p_list(),
                         p_child() {
  sig_finished.connect([this]() {
    std::lock_guard k_guard{_mutex};
    p_fulfil = true;
  });
  sig_message_result.connect([this](const std::string& in_str) {
    if (p_fulfil)
      p_str = in_str;
  });
}

std::double_t long_term::step(std::double_t in_) {
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
  in_forward->sig_progress.connect([this](std::double_t in_pro) { sig_progress(in_pro); });
  in_forward->sig_finished.connect([this]() { sig_finished(); });
  in_forward->sig_message_result.connect([this](const std::string& in_str) { sig_message_result(in_str); });
  p_child.push_back(in_forward);
}

void long_term::forward_sig(const std::vector<long_term_ptr>& in_forward) {
  const std::size_t k_size = in_forward.size();
  for (const auto& i : in_forward) {
    p_child.push_back(i);

    i->sig_progress.connect([this, k_size](
                                std::double_t in_) {
      sig_progress(in_ / boost::numeric_cast<std::double_t>(k_size));
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
}  // namespace doodle
