//
// Created by TD on 2024/3/14.
//

#pragma once
#include <boost/asio.hpp>
namespace doodle::http {
class http_snapshot {
  using timer_type = boost::asio::steady_timer;
  std::shared_ptr<timer_type> timer_;
  void run_impl();
  void do_wait();

 public:
  http_snapshot()  = default;
  ~http_snapshot() = default;

  void run();
};
}  // namespace doodle::http