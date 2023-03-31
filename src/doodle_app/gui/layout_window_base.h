//
// Created by td_main on 2023/3/28.
//
#pragma once

#include <doodle_app/doodle_app_fwd.h>

#include <imgui.h>
#include <string>
namespace doodle::gui::details {

class layout_window_base {
 private:
  std::once_flag once_flag_{};
  std::string name_{"Doodle_DockSpace"};

 protected:
  virtual void layout(ImGuiID in_id, const ImVec2& in_size) = 0;
  virtual std::string& name();

 public:
  layout_window_base() = default;
  bool render();
};

}  // namespace doodle::gui::details
