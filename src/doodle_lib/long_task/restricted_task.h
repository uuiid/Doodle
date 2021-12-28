//
// Created by TD on 2021/12/28.
//

#pragma once
#include <doodle_lib/doodle_lib_fwd.h>

namespace doodle {
class DOODLELIB_API restricted_task :public process_t<restricted_task>{
  class impl;
  std::unique_ptr<impl> p_i;
 public:
  using base_type = process_t<restricted_task>;
};
}  // namespace doodle
