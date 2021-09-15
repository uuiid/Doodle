//
// Created by TD on 2021/9/15.
//

#include "main_windwos.h"

#include <DoodleLib/doodle_app.h>
#include <DoodleLib/libWarp/imgui_warp.h>

namespace doodle {
main_windows::main_windows()
    : p_setting_click(std::make_shared<bool>(false)),
      p_quit(std::make_shared<bool>(false)) {
}
void main_windows::frame_render(const bool_ptr& is_show) {
  //  dear::MainMenuBar k_main_menu_bar{};
  //  k_main_menu_bar&& [this]() {
  //    dear::Menu k_menu_file{"file"};
  //    k_menu_file&& [this]() { this->main_menu_file(); };
  //
  //    dear::Menu k_menu_tool{"tool"};
  //    k_menu_tool&& [this] { main_menu_file(); };
  //  };

  imgui::DockSpaceOverViewport(imgui::GetMainViewport());
}
void main_windows::main_menu_file() {
  if (dear::MenuItem(u8"设置")) {
  }
  if (dear::MenuItem(u8"退出")) {
    doodle_app::Get()->p_done = true;
  }
}
void main_windows::main_menu_tool() {
}

}  // namespace doodle
