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

 public:
  attendance_dingding();
  virtual ~attendance_dingding();
  void set_user(const entt::handle& in_handle) override;
  void set_range(const time_point_wrap& in_begin, const time_point_wrap& in_end) override;
  /**
   * @brief 获取考勤资源
   * @param in_get_data
   * @param in_get_update_data
   */
  void get_resources(
      const doodle::dingding::attendance::attendance& in_get_data,
      const dingding::attendance::attendance& in_get_update_data
  );
  void alt_clock(const work_clock& in_alt_workclock);
  /**
   *
   * @return 返回钟表类
   */
  const work_clock& work_clock_attr() const override;
};

}  // namespace doodle::business
