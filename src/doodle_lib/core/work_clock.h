//
// Created by TD on 2022/4/1.
//

#pragma once

#include <doodle_lib/doodle_lib_fwd.h>
#include <chrono>

namespace doodle {
namespace business {

class DOODLELIB_API adjust_work {
 public:
  chrono::local_time_pos start_;
  chrono::local_time_pos end_;
};

class DOODLELIB_API adjust_rest {
 public:
  chrono::local_time_pos start_;
  chrono::local_time_pos end_;
};

class DOODLELIB_API rules {
 public:
  std::set<chrono::weekday> work_weekdays;
  std::set<
      std::pair<
          chrono::seconds,
          chrono::seconds>>
      work_pair;
  std::vector<adjust_work> extra_work;
  std::vector<adjust_rest> extra_rest;

};
}  // namespace business

chrono::hours_double work_duration(
    const chrono::local_time_pos& in_s,
    const chrono::local_time_pos& in_e,
    const business::rules& in_rules);
}  // namespace doodle
