//
// Created by TD on 2021/9/15.
//

#include "main_windwos.h"

#include <DoodleLib/Gui/setting_windows.h>
#include <DoodleLib/Gui/widgets/project_widget.h>
#include <DoodleLib/doodle_app.h>
#include <DoodleLib/libWarp/imgui_warp.h>

namespace doodle {
main_windows::main_windows()
    : p_setting_show(std::make_shared<bool>(false)),
      p_debug_show(std::make_shared<bool>(false)),
      p_about_show(std::make_shared<bool>(false)),
      p_style_show(std::make_shared<bool>(false)),
      p_quit(std::make_shared<bool>(false)),
      p_title(fmt::format(
          u8"doodle {}.{}.{}.{}",
          Doodle_VERSION_MAJOR,
          Doodle_VERSION_MINOR,
          Doodle_VERSION_PATCH,
          Doodle_VERSION_TWEAK)),
      p_setting(std::make_shared<setting_windows>()),
      p_prj(std::make_shared<project_widget>()) {
}
void main_windows::frame_render(const bool_ptr& is_show) {
  if (*p_setting_show)
    p_setting->frame_render(p_setting_show);
  if (*p_debug_show) imgui::ShowMetricsWindow();
  if (*p_about_show) imgui::ShowAboutWindow();
  if (*p_style_show) {
    dear::Begin{"界面样式编辑", p_style_show.get()} && []() {
      imgui::ShowStyleEditor();
    };
  }

  dear::Begin{
      p_title.c_str(),
      is_show.get(),
      ImGuiWindowFlags_MenuBar} &&
      [this]() {
        dear::MenuBar{} && [this]() {
          dear::Menu{"文件"} && [this]() { this->main_menu_file(); };
          dear::Menu{"工具"} && [this] { main_menu_tool(); };
        };
        p_prj->frame_render();
      };
}
void main_windows::main_menu_file() {
  dear::MenuItem(u8"设置", p_setting_show.get());
  ImGui::Separator();
  dear::MenuItem(u8"调试", p_debug_show.get());
  dear::MenuItem(u8"样式设置", p_style_show.get());
  dear::MenuItem(u8"关于", p_about_show.get());
  ImGui::Separator();
  if (dear::MenuItem(u8"退出")) {
    doodle_app::Get()->p_done = true;
  }
}
void main_windows::main_menu_tool() {
}

}  // namespace doodle
