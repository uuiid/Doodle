//
// Created by TD on 2022/1/20.
//

#pragma once

#include <doodle_app/doodle_app_fwd.h>
#include <doodle_app/gui/base/base_window.h>

namespace doodle::gui {

class DOODLE_APP_API create_project_dialog {
  class impl;
  std::unique_ptr<impl> p_i;
  bool open;

 public:
  explicit create_project_dialog();

  virtual ~create_project_dialog();

  void set_attr() const;
  virtual const std::string& title() const;
  std::int32_t flags() const;
  bool render();
};

class DOODLE_APP_API close_exit_dialog {
  class impl;
  std::unique_ptr<impl> p_i;
  bool open{true};
  std::once_flag once_flag;

 protected:
 public:
  void set_attr() const;
  virtual const std::string& title() const;
  explicit close_exit_dialog();
  virtual ~close_exit_dialog();
  std::int32_t flags() const;
  bool render();

  boost::signals2::signal<void()> quit;
};

}  // namespace doodle::gui
