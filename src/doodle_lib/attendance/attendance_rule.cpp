//
// Created by TD on 2022/10/21.
//

#include "attendance_rule.h"

#include "doodle_core/metadata/time_point_wrap.h"
#include <doodle_core/metadata/detail/time_point_info.h>
#include <doodle_core/time_tool/work_clock.h>

#include "doodle_lib/core/holidaycn_time.h"

#include <fmt/core.h>

namespace doodle::business {

class attendance_rule::impl {
 public:
  work_clock time_clock{};

  entt::handle user_handle{};

  time_point_wrap begin{};
  time_point_wrap end{};
};

attendance_rule::attendance_rule() : ptr(std::make_unique<impl>()) {}

void attendance_rule::set_user(const entt::handle& in_handle) {
  ptr->user_handle = in_handle;
  if (!ptr->user_handle.any_of<rules>()) {
    ptr->user_handle.emplace<rules>(rules::get_default());
  }
}

void attendance_rule::set_range(const time_point_wrap& in_begin, const time_point_wrap& in_end) {
  ptr->begin      = in_begin;
  ptr->end        = (in_end + chrono::days{1});
}

const work_clock& attendance_rule::work_clock_attr() const { return ptr->time_clock; }
void attendance_rule::gen_work_clock() {
  ptr->time_clock = {};
  if (ptr->user_handle && ptr->user_handle.any_of<rules>()) {
    const auto& l_rule = ptr->user_handle.get<rules>();
    // DOODLE_LOG_INFO("时间 {}", l_rule);

    for (auto l_b = ptr->begin; l_b <= ptr->end; l_b += chrono::days{1}) {
      /// \brief 加入工作日规定时间
      if (l_rule.work_weekdays()[l_b.get_week_int()]) {
        ranges::for_each(l_rule.work_time(), [&](const std::pair<chrono::seconds, chrono::seconds>& in_pair) {
          ptr->time_clock += std::make_tuple(l_b + in_pair.first, l_b + in_pair.second);
        });
      }
    }

    /// \brief 调整节假日
    holidaycn_time{}.set_clock(ptr->time_clock);
    // DOODLE_LOG_INFO("时间规则 {}", ptr->time_clock.debug_print());
    /// \brief 减去调休
    // for (auto&& [l_1, l_2, l_3] : l_rule.extra_rest()) {
    //   ptr->time_clock -= std::make_tuple(l_1, l_2, l_3);
    //   DOODLE_LOG_INFO("时间 {}", l_rule);
    //   DOODLE_LOG_INFO("时间规则 {}", ptr->time_clock.debug_print());
    // }
    ranges::for_each(l_rule.extra_rest(), [&](const std::decay_t<decltype(l_rule.extra_rest())>::value_type& in_) {
      ptr->time_clock -= std::make_tuple(in_.first, in_.second, in_.info);
    });
    // DOODLE_LOG_INFO("时间规则2 {}", ptr->time_clock.debug_print());
    /// \brief 加上加班
    ranges::for_each(l_rule.extra_work(), [&](const std::decay_t<decltype(l_rule.extra_work())>::value_type& in_) {
      ptr->time_clock += std::make_tuple(in_.first, in_.second, in_.info);
    });
    // DOODLE_LOG_INFO("时间规则3 {}", ptr->time_clock.debug_print());
    // 去除绝对排除时间
    for (auto l_b = ptr->begin; l_b <= ptr->end; l_b += chrono::days{1}) {
      ranges::for_each(
          l_rule.absolute_deduction[l_b.get_week_int()],
          [&](const std::pair<chrono::seconds, chrono::seconds>& in_pair) {
            ptr->time_clock -= std::make_tuple(l_b + in_pair.first, l_b + in_pair.second);
          }
      );
    }
  }
}
void attendance_rule::async_run(
    const entt::handle& in_handle, const time_point_wrap& in_begin, const time_point_wrap& in_end,
    const detail::attendance_interface::call_type_ptr& in_call_type_ptr
) {
  set_user(in_handle);
  set_range(in_begin, in_end);

  gen_work_clock();
  (*in_call_type_ptr)({}, ptr->time_clock);
}
attendance_rule::~attendance_rule() = default;
}  // namespace doodle::business