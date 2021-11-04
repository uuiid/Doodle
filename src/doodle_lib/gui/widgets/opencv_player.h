//
// Created by TD on 2021/11/04.
//

#pragma once

#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_lib/gui/base_windwos.h>

namespace doodle {
class DOODLELIB_API opencv_player : public base_widget {
  public:
  opencv_player();
  void frame_render() override;
};
}  // namespace doodle
