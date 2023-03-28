//
// Created by td_main on 2023/3/28.
//
#pragma once

#include <doodle_app/doodle_app_fwd.h>

#include <imgui.h>
namespace doodle::gui::details {

class layout_window_base {
 private:
  std::once_flag once_flag_{};
  std::once_flag once_flag_show_{};

  virtual void layout(ImGuiID in_id, const ImVec2& in_size) = 0;
  virtual void init_windows()                               = 0;

 public:
  layout_window_base() = default;
  bool render();
};

}  // namespace doodle::gui::details
