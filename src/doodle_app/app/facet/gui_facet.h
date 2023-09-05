//
// Created by TD on 2022/9/30.
//

#pragma once
#include <doodle_core/core/app_facet.h>
#include <doodle_core/doodle_core.h>
#include <doodle_core/gui_template/show_windows.h>
#include <doodle_core/platform/win/windows_alias.h>

#include <doodle_app/doodle_app_fwd.h>

#include <entt/entt.hpp>
#include <tuple>

namespace doodle::win {
class drop_manager;
}

namespace doodle::gui {
class windows_manage;
}

namespace doodle::facet {

class DOODLE_APP_API gui_facet {
  class impl;
  std::unique_ptr<impl> p_i;

  gui::windows_manage* windows_manage_;

  friend class doodle::gui::windows_manage;

 protected:
  virtual bool translate_message();
  virtual void tick();

  ::doodle::win::wnd_handle p_hwnd{nullptr};
  ::doodle::win::wnd_class p_win_class;
  void init_windows();

  virtual void load_windows() = 0;

 public:
  gui_facet();
  virtual ~gui_facet();

  virtual void show_windows() const;
  virtual void close_windows();
  virtual void destroy_windows();
  win::drop_manager* drop_manager();

  void set_title(const std::string& in_title) const;

  [[nodiscard]] const std::string& name() const noexcept;
  bool post();
  void deconstruction();
  virtual void add_program_options(){};
};
}  // namespace doodle::facet
