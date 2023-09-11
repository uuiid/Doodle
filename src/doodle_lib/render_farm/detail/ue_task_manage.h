//
// Created by td_main on 2023/8/16.
//
#pragma once
#include <boost/asio.hpp>

#include <memory>
namespace doodle {
namespace render_farm {

class ue_task_manage {
 public:
  ue_task_manage()  = default;
  ~ue_task_manage() = default;
  void run();
  inline void cancel() { timer_->cancel(); }

 private:
  using timer = boost::asio::system_timer;
  std::shared_ptr<timer> timer_;
};

}  // namespace render_farm
}  // namespace doodle
