//
// Created by TD on 2022/5/11.
//

#pragma once

#include <chrono>
#include <date/date.h>

namespace doodle::chrono {
namespace literals {
using namespace std::chrono_literals;
using namespace date::literals;

}  // namespace literals
using namespace std::chrono;
using namespace date;

using hours_double   = duration<std::double_t, std::ratio<3600>>;
using sys_time_pos   = time_point<system_clock>;
using local_time_pos = time_point<local_t, seconds>;

template <class dur>
std::time_t to_time_t(const time_point<local_t, dur>& in_timePoint) {
  return duration_cast<seconds>(in_timePoint.time_since_epoch()).count();
};
};  // namespace doodle::chrono
