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
};

}  // namespace doodle::business
