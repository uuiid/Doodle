//
// Created by TD on 2022/1/20.
//

#pragma once

#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_lib/gui/gui_ref/base_window.h>

namespace doodle {

class DOODLELIB_API get_input_project_dialog : public gui::modal_window {
  class impl;
  std::unique_ptr<impl> p_i;

 protected:
  void render() override;

 public:
  explicit get_input_project_dialog(std::shared_ptr<FSys::path> in_handle);
  ~get_input_project_dialog() override;
  virtual void init();
  virtual void succeeded();
};

namespace gui::input {
class DOODLELIB_API get_bool_dialog : public modal_window {
  class impl;
  std::unique_ptr<impl> p_i;

 protected:
  void render() override;

 public:
  explicit get_bool_dialog(const std::shared_ptr<bool>& is_quit);
};
}  // namespace gui::input
}  // namespace doodle
