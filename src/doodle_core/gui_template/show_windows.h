//
// Created by TD on 2022/9/21.
//

#pragma once

#include <doodle_core/doodle_core_fwd.h

namespace doodle::detail {

class tack_proc_interface {
 public:
  virtual ~tack_proc_interface()              = default;

  [[nodiscard("")]] virtual bool operator()() = 0;
};

template <typename T>
class tack_proc {
 public:
};
}  // namespace doodle::detail
