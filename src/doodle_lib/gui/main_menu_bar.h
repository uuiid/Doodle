//
// Created by TD on 2022/1/14.
//

#pragma once
#include <doodle_lib/doodle_lib_fwd.h>

namespace doodle {
class DOODLELIB_API main_menu_bar : public process_t<main_menu_bar> {
 private:
  class impl;
  std::unique_ptr<impl> p_i;

 protected:
  void open_by_name_widget(const std::string_view& in_view);
  virtual void menu_file();
  virtual void menu_windows();
  virtual void menu_edit();
  virtual void menu_tool();

 public:
  main_menu_bar();
  ~main_menu_bar() override;

  [[maybe_unused]] void init();
  [[maybe_unused]] void succeeded();
  [[maybe_unused]] void failed();
  [[maybe_unused]] void aborted();
  [[maybe_unused]] void update(delta_type, void* data);
};
}  // namespace doodle
