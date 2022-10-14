//
// Created by TD on 2022/9/30.
//

#pragma once
#include <doodle_app/doodle_app_fwd.h>

#include <doodle_core/doodle_core.h>
#include <doodle_core/core/app_facet.h>

#include <doodle_core/platform/win/windows_alias.h>
namespace doodle::facet {

class DOODLE_APP_API gui_facet : public ::doodle::detail::app_facet_interface {
  class impl;
  std::unique_ptr<impl> p_i;

 protected:
  virtual void tick_begin();
  virtual void tick();
  virtual void tick_end();

  ::doodle::win::wnd_handle p_hwnd;
  ::doodle::win::wnd_class p_win_class;
  void post_constructor();

  virtual void load_windows() = 0;

 public:
  gui_facet();
  virtual ~gui_facet() override;

  virtual void show_windows() const;
  virtual void close_windows();
  void set_title(const std::string& in_title) const;

  [[nodiscard]] const std::string& name() const noexcept override;
  void operator()() override;
  void deconstruction() override;
};
}  // namespace doodle::facet
