//
// Created by TD on 2021/9/15.
//

#include "main_windwos.h"

#include <DoodleLib/Gui/setting_windows.h>
#include <DoodleLib/doodle_app.h>
#include <DoodleLib/libWarp/imgui_warp.h>
namespace doodle {
main_windows::main_windows()
    : p_setting_show(std::make_shared<bool>(false)),
      p_quit(std::make_shared<bool>(false)),
      p_title(fmt::format(
          u8"doodle {}.{}.{}.{}",
          Doodle_VERSION_MAJOR,
          Doodle_VERSION_MINOR,
          Doodle_VERSION_PATCH,
          Doodle_VERSION_TWEAK)),
      p_setting(std::make_shared<setting_windows>()) {
}
void main_windows::frame_render(const bool_ptr& is_show) {
  if (*p_setting_show)
    p_setting->frame_render(p_setting_show);

  dear::Begin{
      p_title.c_str(),
      is_show.get(),
      ImGuiWindowFlags_MenuBar} &&
      [this]() {
        dear::MenuBar{} && [this]() {
          dear::Menu{"file"} && [this]() { this->main_menu_file(); };
          dear::Menu{"tool"} && [this] { main_menu_tool(); };
        };

        ImGui::Spacing();
        ImGui::Text("This is some useful text.");
        ImGui::Text("This is some useful text.2");
        ImGui::Text("This is some useful text.3");
        ImGui::Text("This is some useful text.4");
      };
}
void main_windows::main_menu_file() {
  dear::MenuItem(u8"设置", p_setting_show.get());
  if (dear::MenuItem(u8"退出")) {
    doodle_app::Get()->p_done = true;
  }
}
void main_windows::main_menu_tool() {
}

}  // namespace doodle
