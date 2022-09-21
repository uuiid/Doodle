//
// Created by TD on 2022/1/14.
//

#pragma once
#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_lib/gui/gui_ref/base_window.h>

namespace doodle {
namespace main_menu_bar_ns {
class layout_data {
 private:
  friend void to_json(nlohmann::json& j, const layout_data& p);
  friend void from_json(const nlohmann::json& j, layout_data& p);

 public:
  std::map<std::string, bool> windows_show;
  std::string imgui_data;
  std::string name;
  bool operator==(const layout_data& in_rhs) const;
  bool operator!=(const layout_data& in_rhs) const;
  bool operator==(const std::string& in_rhs) const;
  bool operator!=(const std::string& in_rhs) const;
};
}  // namespace main_menu_bar_ns
class DOODLELIB_API main_menu_bar {
 private:
  class impl;
  std::unique_ptr<impl> p_i;

 protected:
  virtual void menu_file();
  virtual void menu_windows();
  virtual void menu_edit();
  virtual void menu_tool();
  void menu_layout();

 public:
 public:
  main_menu_bar();
  ~main_menu_bar();

  main_menu_bar(const main_menu_bar& in) noexcept;
  main_menu_bar(main_menu_bar&& in) noexcept;
  main_menu_bar& operator=(const main_menu_bar& in) noexcept;
  main_menu_bar& operator=(main_menu_bar&& in) noexcept;

  void render();
};
}  // namespace doodle
