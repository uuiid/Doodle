//
// Created by td_main on 2023/8/22.
//
#pragma once
#include <doodle_app/gui/base/base_window.h>
#include <doodle_app/gui/base/ref_base.h>

#include <doodle_lib/doodle_lib_fwd.h>

#include <cstdint>
namespace doodle {
namespace gui {

class render_monitor {
 private:
  class impl {
    bool open_{true};
    gui_cache_name_id component_table_id_;
    gui_cache_name_id render_task_table_id_;
  };
  std::unique_ptr<impl> p_i;

 public:
  render_monitor() : p_i(std::make_unique<impl>()){};
  ~render_monitor() = default;

  constexpr static std::string_view name{gui::config::menu_w::render_monitor};

  void init();
  bool render();
  const std::string& title() const;
};

}  // namespace gui
}  // namespace doodle
