//
// Created by TD on 2022/10/21.
//
#pragma once

#include <doodle_lib/attendance/attendance_interface.h>

namespace doodle::dingding::attendance {
class attendance;
}

namespace doodle::business {

class attendance_dingding : public detail::attendance_interface {
 private:
  class impl;
  std::unique_ptr<impl> ptr;

  void get_work_time();

  void do_work();

 protected:
  void async_run(
      const entt::handle& in_handle, const time_point_wrap& in_begin, const time_point_wrap& in_end,
      const call_type_ptr& in_call_type_ptr
  ) override;

 public:
  attendance_dingding();
  virtual ~attendance_dingding();
  void set_user(const entt::handle& in_handle) override;
  void set_range(const time_point_wrap& in_begin, const time_point_wrap& in_end) override;
};

}  // namespace doodle::business
