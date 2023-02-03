#pragma once

#include <doodle_core/configure/static_value.h>

#include <doodle_app/gui/base/base_window.h>

#include <doodle_lib/doodle_lib_fwd.h>

#include <entt/entity/fwd.hpp>
#include <vector>

namespace doodle::gui {
class DOODLELIB_API time_sequencer_widget : public base_windows<dear::Begin, time_sequencer_widget> {
  class impl;
  std::unique_ptr<impl> p_i;

  void fliter_select( );
  void gen_user();
  void refresh_work_rule();

 public:
  time_sequencer_widget();
  ~time_sequencer_widget() override;

  constexpr static std::string_view name = config::menu_w::time_edit;
  [[nodiscard]] const std::string& title() const override;
  void render();
};
}  // namespace doodle::gui
