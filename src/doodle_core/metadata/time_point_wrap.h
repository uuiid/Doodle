//
// Created by TD on 2021/5/17.
//

#pragma once

#include <doodle_core/doodle_core_fwd.h>

#include <boost/operators.hpp>
#include <boost/pfr.hpp>
#include <boost/pfr/functions_for.hpp>

#include <chrono>
#include <fmt/chrono.h>

namespace doodle {
class time_point_wrap;

template <typename TimePoint_T>
auto parse_8601(const std::string& in_str, boost::system::error_code& ec) {
  TimePoint_T l_time_point;
  std::istringstream l_stream{in_str};
  l_stream >> std::chrono::parse("%FT%T", l_time_point);
  // if (in_str.back() == 'Z') {
  // } else {
  //   l_stream >> std::chrono::parse("%FT%T%Ez", l_time_point);
  // }
  if (!l_stream) {
    ec = boost::system::error_code{ERROR_CLUSTER_INVALID_STRING_FORMAT, boost::system::system_category()};
  }
  return l_time_point;
}
template <typename TimePoint_T>
auto parse_8601(const std::string& in_str) {
  boost::system::error_code l_ec;
  auto l_time_point = parse_8601<TimePoint_T>(in_str, l_ec);
  if (l_ec) {
    throw std::runtime_error(l_ec.message());
  }
  return l_time_point;
}

}  // namespace doodle

#include "doodle_core/lib_warp/chrono_fmt.h"