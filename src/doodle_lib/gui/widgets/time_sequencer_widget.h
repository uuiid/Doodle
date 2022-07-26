#pragma once

#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_lib/gui/gui_ref/base_window.h>

namespace doodle::gui {
class DOODLELIB_API time_sequencer_widget
    : public window_panel {
  class impl;
  std::unique_ptr<impl> p_i;

 public:
  time_sequencer_widget();
  ~time_sequencer_widget() override;

  constexpr static std::string_view name{"时间序列"};

  void render() override;
};
}  // namespace doodle::gui
