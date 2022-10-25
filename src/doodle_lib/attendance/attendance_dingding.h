//
// Created by TD on 2022/10/21.
//
#pragma once

#include <doodle_lib/attendance/attendance_interface.h>
namespace doodle::business {

class attendance_dingding : public detail::attendance_interface {
 private:
  class impl;
  std::unique_ptr<impl> ptr;

 public:
  attendance_dingding();
  virtual ~attendance_dingding();
  void set_user(const entt::handle& in_handle) override;
  void set_range(const time_point_wrap& in_begin, const time_point_wrap& in_end) override;
  void get_resources(const dingding::attendance::attendance& get_data)override;
  void alt_clock(const work_clock& alt_workclock)override;
  const work_clock& work_clock_attr() const override;
};

}  // namespace doodle::business
