//
// Created by TD on 2021/12/6.
//

#pragma once
#include <doodle_lib/doodle_lib_fwd.h>

namespace doodle::maya_plug {
class reference_file;

class qcloth_shape {
 private:
  entt::handle p_ref_file;

 public:
  qcloth_shape();
  explicit qcloth_shape(const entt::handle& in_ref_file);



};
}  // namespace doodle::maya_plug
