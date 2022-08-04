//
// Created by TD on 2022/8/4.
//

#pragma once

#include <doodle_core/metadata/time_point_wrap.h>

#include <boost/icl/discrete_interval.hpp>
namespace boost::icl {
template <>
struct is_discrete<::doodle::time_point_wrap> {
  typedef is_discrete type;
  BOOST_STATIC_CONSTANT(bool, value = true);
};

template <>
inline ::doodle::time_point_wrap identity_element<::doodle::time_point_wrap>::value() {
  return ::doodle::time_point_wrap::min();
}

template <>
struct identity_element<::doodle::time_point_wrap::duration> {
  static ::doodle::time_point_wrap::duration value() {
    return ::doodle::time_point_wrap::max() - ::doodle::time_point_wrap::min();
  }
};

template <>
struct has_difference<::doodle::time_point_wrap> {
  typedef has_difference type;
  BOOST_STATIC_CONSTANT(bool, value = true);
};

template <>
struct difference_type_of<::doodle::time_point_wrap> {
  typedef ::doodle::time_point_wrap::duration type;
};

template <>
struct size_type_of<::doodle::time_point_wrap> {
  typedef ::doodle::time_point_wrap::duration type;
};

// ------------------------------------------------------------------------
template <>
struct is_discrete<::doodle::time_point_wrap::duration> {
  typedef is_discrete type;
  BOOST_STATIC_CONSTANT(bool, value = true);
};

template <>
struct has_difference<::doodle::time_point_wrap::duration> {
  typedef has_difference type;
  BOOST_STATIC_CONSTANT(bool, value = true);
};

template <>
struct size_type_of<::doodle::time_point_wrap::duration> {
  typedef ::doodle::time_point_wrap::duration type;
};
// ------------------------------------------------------------------------
inline ::doodle::time_point_wrap::duration operator++(::doodle::time_point_wrap::duration& x) {
  return x++;
}

inline ::doodle::time_point_wrap::duration operator--(::doodle::time_point_wrap::duration& x) {
  return x--;
}

};  // namespace boost::icl
