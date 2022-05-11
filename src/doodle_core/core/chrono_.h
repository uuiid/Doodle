//
// Created by TD on 2022/5/11.
//

#pragma once

#include <chrono>
#include <date/date.h>

namespace doodle {
namespace chrono {
namespace literals {
using namespace std::chrono_literals;
using namespace date::literals;

}  // namespace literals
using namespace std::chrono;
using namespace date;

using hours_double   = duration<std::double_t, std::ratio<3600>>;
using days_double    = duration<std::double_t, std::ratio<28800>>;
using sys_time_pos   = time_point<system_clock>;
using local_time_pos = time_point<local_t, seconds>;

/**
 * @brief 判断是否是休息日 周六日
 *
 * @todo 这里我们暂时使用周六和周日作为判断, 但是实际上还有各种假期和其他情况要计入
 */
bool is_rest_day(const sys_days& in_days);
bool is_rest_day(const local_days& in_days);
template <class dur>
std::time_t to_time_t(const time_point<local_t, dur>& in_timePoint) {
  return duration_cast<seconds>(in_timePoint.time_since_epoch()).count();
};
}  // namespace chrono
}  // namespace doodle
