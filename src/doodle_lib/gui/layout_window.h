//
// Created by TD on 2022/9/29.
//

#pragma once

#include <doodle_app/doodle_app_fwd.h>
#include <doodle_app/gui/base/base_window.h>
#include <doodle_app/gui/layout_window_base.h>

#include <doodle_lib/doodle_lib_fwd.h>

namespace doodle::gui {

class DOODLELIB_API layout_window : public details::layout_window_base {
 protected:
  virtual void layout(ImGuiID in_id, const ImVec2& in_size);
  virtual void init_windows();

 public:
  layout_window();
  virtual ~layout_window();
};

}  // namespace doodle::gui
