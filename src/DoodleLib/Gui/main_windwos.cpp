//
// Created by TD on 2021/9/15.
//

#include "main_windwos.h"

#include <DoodleLib/Exception/Exception.h>
#include <DoodleLib/Gui/setting_windows.h>
#include <DoodleLib/Gui/widgets/assets_widget.h>
#include <DoodleLib/Gui/widgets/attribute_widgets.h>
#include <DoodleLib/Gui/widgets/project_widget.h>
#include <DoodleLib/doodle_app.h>
#include <DoodleLib/libWarp/imgui_warp.h>
#include <DoodleLib/toolkit/toolkit.h>
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
      p_prj(std::make_shared<project_widget>()),
      p_ass(std::make_shared<assets_widget>()),
      p_attr(std::make_shared<attribute_widgets>()) {
  p_prj->select_change.connect([this](auto in) { p_ass->set_metadata(in); });
  p_ass->select_change.connect([this](auto in) { p_attr->set_metadata(in); });
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
          dear::Menu{"工具"} && [this]() { main_menu_tool(); };
        };
        p_prj->frame_render();
        p_ass->frame_render();
        //        imgui::SameLine();
        p_attr->frame_render();
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
  if (dear::MenuItem("安装maya插件"))
    toolkit::installMayaPath();
  if (dear::MenuItem("安装ue4插件"))
    try {
      toolkit::installUePath(CoreSet::getSet().gettUe4Setting().Path() / "Engine");
    } catch (const DoodleError& err) {
      dear::PopupModal{"警告"} && [&err]() {
        dear::Text(err.what());
        if (ImGui::Button("OK", ImVec2(120, 0)))
          ImGui::CloseCurrentPopup();
      };
      DOODLE_LOG_WARN(err.what());
    }

  if (dear::MenuItem("安装ue4项目插件")) {
    imgui::FileDialog::Instance()->OpenModal(
        "ChooseDirDlgKey",
        "select_ue_project",
        ".uproject",
        ".");
    doodle_app::Get()->main_loop.connect_extended([](const doodle_app::connection& in) {
      dear::OpenFileDialog{"ChooseDirDlgKey"} && [in]() {
        auto ig = ImGuiFileDialog::Instance();
        if (ig->IsOk()) {
          FSys::path k_path = ig->GetCurrentPath();
          toolkit::installUePath(k_path);
        }
        in.disconnect();
      };
    });
  }
  if (dear::MenuItem("删除ue4缓存"))
    toolkit::deleteUeCache();
  if (dear::MenuItem("修改ue4缓存位置"))
    toolkit::modifyUeCachePath();
}

}  // namespace doodle
