//
// Created by TD on 2022/9/29.
//

#pragma once

#include <doodle_app/gui/main_menu_bar.h>

#include <doodle_lib/doodle_lib_fwd.h>
namespace doodle::gui {
class DOODLELIB_API menu_bar : public main_menu_bar {
 public:
  static void message(const std::string& in_m);

 protected:
  void menu_tool() override;

 private:
  // 启动渲染客户端
  void menu_start_render_client();
  bool run_client;
};
}  // namespace doodle::gui
