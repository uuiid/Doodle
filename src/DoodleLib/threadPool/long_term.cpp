#include "long_term.h"

#include <boost/numeric/conversion/cast.hpp>
namespace doodle {
long_term::long_term() : sig_progress(),
                         sig_message_result(),
                         sig_finished(),
                         p_fulfil(false),
                         p_str(),
                         p_progress(0),
                         _mutex() {
  sig_finished.connect([this]() { p_fulfil = true; });
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
}  // namespace doodle
