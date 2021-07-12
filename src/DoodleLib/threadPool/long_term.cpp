#include "long_term.h"
namespace doodle {
long_term::long_term() : sig_progress(),
                         sig_message_result(),
                         sig_finished(),
                         p_fulfil(false) {
  sig_finished.connect([this]() { p_fulfil = true; });
}

bool long_term::fulfil() const {
  return p_fulfil;
}
}  // namespace doodle
