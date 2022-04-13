//
// Created by TD on 2022/1/14.
//

#pragma once
#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_lib/gui/gui_ref/base_window.h>
namespace doodle {
class DOODLELIB_API main_menu_bar
    : public gui::base_window,
      public process_t<main_menu_bar> {
 private:
  class impl;
  std::unique_ptr<impl> p_i;

 protected:
  void open_by_name_widget(const std::string_view& in_view);
  virtual void menu_file();
  virtual void menu_windows();
  virtual void menu_edit();
  virtual void menu_tool();
  void menu_layout();
  void layout_save();
  void layout_load();
  void layout_list();

 public:
  main_menu_bar();
  ~main_menu_bar() override;

  const string& title() const override;
  void init() override;
  void succeeded() override;
  virtual void aborted() override;
  void update(
      const chrono::system_clock::duration& in_duration,
      void* in_data) override;
};
}  // namespace doodle
