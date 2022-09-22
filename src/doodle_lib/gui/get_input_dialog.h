//
// Created by TD on 2022/1/20.
//

#pragma once

#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_lib/gui/gui_ref/base_window.h>

namespace doodle::gui {

class DOODLELIB_API create_project_dialog
    : public base_windows<dear::PopupModal, create_project_dialog> {
  class impl;
  std::unique_ptr<impl> p_i;

 public:
  explicit create_project_dialog();

  virtual ~create_project_dialog();

  void set_attr() const;
  virtual const std::string& title() const override;
  std::int32_t flags() const;
  void render();
};

class DOODLELIB_API close_exit_dialog
    : public base_windows<dear::PopupModal, close_exit_dialog> {
  class impl;
  std::unique_ptr<impl> p_i;

 protected:
 public:
  void set_attr() const;
  virtual const std::string& title() const override;
  explicit close_exit_dialog();
  std::int32_t flags() const;
  void render();
};

}  // namespace doodle::gui
