//
// Created by td_main on 2023/3/31.
//
#pragma once
#include <doodle_app/gui/layout_window_base.h>
namespace doodle::gui {

class asset_library_layout : public details::layout_window_base {
 public:
  asset_library_layout() = default;

  void set_show();

 protected:
  void layout(ImGuiID in_id, const ImVec2& in_size) override;
};

}  // namespace doodle::gui
