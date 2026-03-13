#include "cancellation_signals.h"
namespace doodle {
void cancellation_signals::sweep_locked() {
  for (auto it = sigs.begin(); it != sigs.end();) {
    auto l_slot      = it->signal.slot();
    auto l_has_owner = l_slot.has_handler();

    if (l_has_owner) {
      it->observed_handler = true;
      ++it;
      continue;
    }

    if (it->observed_handler)
      it = sigs.erase(it);
    else
      ++it;
  }
}

void cancellation_signals::emit(boost::asio::cancellation_type ct) {
  std::lock_guard _(mtx);

  sweep_locked();

  for (auto& sig : sigs) sig.signal.emit(ct);
}

boost::asio::cancellation_slot cancellation_signals::slot() {
  std::lock_guard _(mtx);

  sweep_locked();

  auto& l_entry = sigs.emplace_back();
  return l_entry.signal.slot();
}
}  // namespace doodle