#pragma once
#include <doodle_lib/doodle_lib_fwd.h>

namespace doodle {
class cancellation_signals {
  struct signal_entry {
    boost::asio::cancellation_signal signal;
    bool observed_handler{false};
  };

  std::list<signal_entry> sigs;
  std::mutex mtx;

  void sweep_locked();

 public:
  void emit(boost::asio::cancellation_type ct = boost::asio::cancellation_type::all);

  boost::asio::cancellation_slot slot();
};
}  // namespace doodle