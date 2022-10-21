//
// Created by TD on 2022/10/21.
//

#pragma once

#include <doodle_core/time_tool/work_clock.h>

#include <doodle_lib/doodle_lib_fwd.h>

namespace doodle {
namespace detail {

class attendance_interface {
 public:
  attendance_interface()                                                  = default;
  virtual const doodle::business::work_clock& work_clock() const noexcept = 0;
};

}  // namespace detail
}  // namespace doodle
