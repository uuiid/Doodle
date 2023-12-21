//
// Created by TD on 2023/12/21.
//
#pragma once

#include <doodle_app/doodle_app_fwd.h>
#include <doodle_app/gui/layout_window_base.h>
namespace doodle::gui {

class create_video_layout : public details::layout_window_base {
 public:
  create_video_layout() = default;
  void set_show();
  static constexpr auto name{"创建视频工作区"};

 protected:
  void layout(ImGuiID in_id, const ImVec2& in_size) override;
};

}  // namespace doodle::gui