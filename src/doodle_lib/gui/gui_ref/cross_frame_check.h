//
// Created by TD on 2022/8/1.
//

#pragma once

#include <doodle_lib/doodle_lib_fwd.h>
namespace doodle::gui::detail {

template <typename Cache_T>
class cross_frame_check {
 public:
  Cache_T data;

  //  std::

  cross_frame_check()          = default;
  virtual ~cross_frame_check() = default;
};

}  // namespace doodle::gui::detail
