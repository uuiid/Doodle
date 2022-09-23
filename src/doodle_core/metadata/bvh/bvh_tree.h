//
// Created by TD on 2022/9/23.
//

#pragma once

#include <doodle_core/doodle_core_fwd.h>

namespace doodle::bvh {
class DOODLE_CORE_API bvh_tree {
 private:
  class impl;

  std::unique_ptr<impl> p_i;

 public:
  bvh_tree();
  virtual ~bvh_tree();

  void parse(const FSys::ifstream& in_stream);
};
}  // namespace doodle::bvh
