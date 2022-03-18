//
// Created by TD on 2021/5/18.
//

#pragma once

#include <doodle_lib/doodle_lib_fwd.h>
namespace doodle {
class importance {
 private:
 public:
  std::string cutoff_p;
  importance();
  importance(std::string in_cutoff_p);
  ~importance();
};
}  // namespace doodle
