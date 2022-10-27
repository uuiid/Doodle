//
// Created by TD on 2022/10/21.
//

#pragma once

#include <doodle_core/metadata/time_point_wrap.h>

#include <doodle_lib/attendance/attendance_interface.h>
namespace doodle::business {

class attendance_rule : public detail::attendance_interface {
 private:
  class impl;
  std::unique_ptr<impl> ptr;

  void gen_work_clock();

 protected:
  void async_run(const call_type_ptr& in_call_type_ptr) override;

 public:
  attendance_rule();
  virtual ~attendance_rule();

  void set_user(const entt::handle& in_handle) override;

  void set_range(const time_point_wrap& in_begin, const time_point_wrap& in_end) override;
  [[nodiscard]] const work_clock& work_clock_attr() const override;
};

}  // namespace doodle::business
