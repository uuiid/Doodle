//
// Created by TD on 2022/6/1.
//

#pragma once

#include <type_traits>
#include <boost/numeric/conversion/cast.hpp>
namespace doodle {
template <typename Enum_T, typename Num_T>
Enum_T num_to_enum(Num_T in_t) {
  return static_cast<Enum_T>(boost::numeric_cast<typename std::underlying_type_t<Enum_T>>(in_t));
}

template <typename Enum_T>
auto enum_to_num(Enum_T in_t) {
  return static_cast<typename std::underlying_type_t<Enum_T>>(in_t);
}
}  // namespace doodle
