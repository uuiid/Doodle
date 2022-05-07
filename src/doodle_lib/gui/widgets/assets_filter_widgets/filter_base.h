//
// Created by TD on 2022/5/7.
//

#pragma once

#include <doodle_lib/doodle_lib_fwd.h>

namespace doodle::gui{
class DOODLELIB_API filter_base {
 public:
  virtual ~filter_base()                                = default;
  virtual bool operator()(const entt::handle& in) const = 0;
};
}
