//
// Created by TD on 2022/6/16.
//

#pragma once

#include <boost/icl/iterator.hpp>
#include <boost/icl/discrete_interval.hpp>

#include <doodle_core/core/chrono_.h>
namespace boost::icl {
template <>
struct is_discrete<doodle::chrono::local_time_pos> {
  typedef is_discrete type;
  BOOST_STATIC_CONSTANT(bool, value = true);
};

template <>
inline doodle::chrono::local_time_pos identity_element<doodle::chrono::local_time_pos>::value() {
  return doodle::chrono::local_time_pos::min();
}

template <>
struct identity_element<doodle::chrono::local_time_pos::duration> {
  static doodle::chrono::local_time_pos::duration value() {
    return doodle::chrono::local_time_pos::max() - doodle::chrono::local_time_pos::min();
  }
};

template <>
struct has_difference<doodle::chrono::local_time_pos> {
  typedef has_difference type;
  BOOST_STATIC_CONSTANT(bool, value = true);
};

template <>
struct difference_type_of<doodle::chrono::local_time_pos> { typedef doodle::chrono::local_time_pos::duration type; };

template <>
struct size_type_of<doodle::chrono::local_time_pos> { typedef doodle::chrono::local_time_pos::duration type; };

// ------------------------------------------------------------------------
inline doodle::chrono::local_time_pos operator++(doodle::chrono::local_time_pos& x) {
  return doodle::chrono::local_time_pos{++x.time_since_epoch()};
}

inline doodle::chrono::local_time_pos operator--(doodle::chrono::local_time_pos& x) {
  return doodle::chrono::local_time_pos{--x.time_since_epoch()};
}
// ------------------------------------------------------------------------
template <>
struct is_discrete<doodle::chrono::local_time_pos::duration> {
  typedef is_discrete type;
  BOOST_STATIC_CONSTANT(bool, value = true);
};

template <>
struct has_difference<doodle::chrono::local_time_pos::duration> {
  typedef has_difference type;
  BOOST_STATIC_CONSTANT(bool, value = true);
};

template <>
struct size_type_of<doodle::chrono::local_time_pos::duration> {
  typedef doodle::chrono::local_time_pos::duration type;
};
// ------------------------------------------------------------------------
inline doodle::chrono::local_time_pos::duration operator++(doodle::chrono::local_time_pos::duration& x) {
  return x++;
}

inline doodle::chrono::local_time_pos::duration operator--(doodle::chrono::local_time_pos::duration& x) {
  return x--;
}

};  // namespace boost::icl

