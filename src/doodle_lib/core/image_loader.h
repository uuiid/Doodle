//
// Created by TD on 2022/1/21.
//

#pragma once
#include <doodle_lib/doodle_lib_fwd.h>
namespace doodle {
class DOODLELIB_API image_loader {
  class impl;
  std::unique_ptr<impl> p_i;

 public:
  image_loader();
  virtual ~image_loader();
};
}  // namespace doodle
