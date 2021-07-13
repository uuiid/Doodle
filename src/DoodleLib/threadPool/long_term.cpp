#include "long_term.h"
namespace doodle {
long_term::long_term() : sig_progress(),
                         sig_message_result(),
                         sig_finished(),
                         p_fulfil(false),
                         p_str() {
  sig_finished.connect([this]() { p_fulfil = true; });
  sig_message_result.connect([this](const std::string& in_str) {
    if (p_fulfil)
      p_str = in_str;
  });
}

bool long_term::fulfil() const {
  return p_fulfil;
}

std::string long_term::message_result() const {
  return p_str;
}
}  // namespace doodle
