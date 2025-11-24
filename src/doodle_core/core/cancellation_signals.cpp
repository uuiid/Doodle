#include "cancellation_signals.h"
namespace doodle {
void cancellation_signals::emit(boost::asio::cancellation_type ct) {
  std::lock_guard _(mtx);

  for (auto& sig : sigs) sig.emit(ct);
}

boost::asio::cancellation_slot cancellation_signals::slot() {
  std::lock_guard _(mtx);

  auto itr = std::find_if(sigs.begin(), sigs.end(), [](boost::asio::cancellation_signal& sig) {
    return !sig.slot().has_handler();
  });

  if (itr != sigs.end()) return itr->slot();
  return sigs.emplace_back().slot();
}
}  // namespace doodle