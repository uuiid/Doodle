//
// Created by TD on 2021/9/15.
//

#include "main_windwos.h"

#include <DoodleLib/doodle_app.h>
#include <DoodleLib/libWarp/imgui_warp.h>

namespace doodle {
main_windows::main_windows()
    : p_setting_click(std::make_shared<bool>(false)),
      p_quit(std::make_shared<bool>(false)),
      p_title(fmt::format(
          u8"doodle {}.{}.{}.{}",
          Doodle_VERSION_MAJOR,
          Doodle_VERSION_MINOR,
          Doodle_VERSION_PATCH,
          Doodle_VERSION_TWEAK)) {
}
void main_windows::frame_render(const bool_ptr& is_show) {
  dear::Begin{
      p_title.c_str(),
      is_show.get(),
      ImGuiWindowFlags_MenuBar} &&
      [this]() {
        dear::MenuBar{} && [this]() {
          dear::Menu k_menu_file{"file"};
          k_menu_file&& [this]() { this->main_menu_file(); };
          dear::Menu k_menu_tool{"tool"};
          k_menu_tool&& [this] { main_menu_tool(); };
        };

        ImGui::Spacing();
        ImGui::Text("This is some useful text.");
        ImGui::Text("This is some useful text.2");
        ImGui::Text("This is some useful text.3");
        ImGui::Text("This is some useful text.4");
      };
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
