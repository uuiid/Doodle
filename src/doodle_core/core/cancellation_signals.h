#pragma once
#include <doodle_core/doodle_core_fwd.h>

namespace doodle {
class cancellation_signals {
  std::list<boost::asio::cancellation_signal> sigs;
  std::mutex mtx;

 public:
  void emit(boost::asio::cancellation_type ct = boost::asio::cancellation_type::all);

  boost::asio::cancellation_slot slot();
};
}  // namespace doodle