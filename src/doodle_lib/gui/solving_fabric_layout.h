//
// Created by td_main on 2023/4/24.
//
#pragma once

#include <doodle_app/doodle_app_fwd.h>
#include <doodle_app/gui/layout_window_base.h>
namespace doodle::gui {

class solving_fabric_layout : public details::layout_window_base {
 public:
  solving_fabric_layout() = default;
  void set_show();
  static constexpr auto name{"解算工作区"};

 protected:
  void layout(ImGuiID in_id, const ImVec2& in_size) override;
};

}  // namespace doodle::gui
