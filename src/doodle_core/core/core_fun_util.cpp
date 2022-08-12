//
// Created by TD on 2022/4/28.
//
#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/exception/exception.h>
#include <doodle_core/logger/logger.h>

#include <boost/algorithm/string.hpp>

#include <Windows.h>
#include <shellapi.h>
#include <tchar.h>

namespace doodle {

namespace chrono {
bool is_rest_day(const sys_days &in_days) {
  weekday k_weekday{in_days};
  return k_weekday == Sunday || k_weekday == Saturday;
}
bool is_rest_day(const local_days &in_days) {
  weekday k_weekday{in_days};
  return k_weekday == Sunday || k_weekday == Saturday;
}
}  // namespace chrono

}  // namespace doodle

// namespace doodle::FSys
