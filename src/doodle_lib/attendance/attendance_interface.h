//
// Created by TD on 2022/10/21.
//

#pragma once

#include <doodle_core/time_tool/work_clock.h>

#include <doodle_lib/doodle_lib_fwd.h>

namespace doodle::business::detail {

class attendance_interface {
 public:
  attendance_interface()                                                                 = default;
  virtual ~attendance_interface()                                                        = default;
  virtual void set_user(const entt::handle& in_handle)                                   = 0;
  virtual void set_range(const time_point_wrap& in_begin, const time_point_wrap& in_end) = 0;
  virtual const doodle::business::work_clock& work_clock_attr() const                    = 0;
};

}  // namespace doodle::business::detail
