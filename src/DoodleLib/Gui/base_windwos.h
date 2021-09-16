//
// Created by TD on 2021/9/15.
//

#pragma once
#include <DoodleLib/DoodleLib_fwd.h>
namespace doodle {

class DOODLELIB_API base_windows : public details::no_copy {
 public:
  virtual void frame_render(const bool_ptr& is_show) = 0;
};
class DOODLELIB_API base_widget : public details::no_copy {
 public:
  virtual void frame_render() = 0;
};
}  // namespace doodle
