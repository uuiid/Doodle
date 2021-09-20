//
// Created by TD on 2021/9/15.
//

#include "main_windwos.h"

#include <DoodleLib/Exception/Exception.h>
#include <DoodleLib/Gui/action/command_tool.h>
#include <DoodleLib/Gui/setting_windows.h>
#include <DoodleLib/Gui/widgets/assets_widget.h>
#include <DoodleLib/Gui/widgets/attribute_widgets.h>
#include <DoodleLib/Gui/widgets/long_time_tasks_widget.h>
#include <DoodleLib/Gui/widgets/project_widget.h>
#include <DoodleLib/doodle_app.h>
#include <DoodleLib/libWarp/imgui_warp.h>
#include <DoodleLib/toolkit/toolkit.h>
namespace doodle {
main_windows::main_windows()
    : p_setting_show(new_object<bool>(false)),
      p_debug_show(new_object<bool>(false)),
      p_about_show(new_object<bool>(false)),
      p_style_show(new_object<bool>(false)),
      p_long_task_show(new_object<bool>(true)),
      p_title(fmt::format(
          u8"doodle {}.{}.{}.{}",
          Doodle_VERSION_MAJOR,
          Doodle_VERSION_MINOR,
          Doodle_VERSION_PATCH,
          Doodle_VERSION_TWEAK)),
      p_setting(new_object<setting_windows>()),
      p_prj(new_object<project_widget>()),
      p_ass(new_object<assets_widget>()),
      p_attr(new_object<attribute_widgets>()),
      p_long_task(new_object<long_time_tasks_widget>()) {
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
  if (*p_long_task_show) {
    dear::Begin{"后台任务", p_style_show.get()} && [this]() {
      p_long_task->frame_render();
    };
  }

  dear::Begin{
      p_title.c_str(),
      is_show.get(),
      ImGuiWindowFlags_MenuBar} &&
      [this]() {
        dear::MenuBar{} && [this]() {
          dear::Menu{"文件"} && [this]() { this->main_menu_file(); };
          dear::Menu{"窗口"} && [this]() { this->main_menu_windows(); };
          dear::Menu{"编辑"} && [this]() { this->main_menu_edit(); };
          dear::Menu{"工具"} && [this]() { this->main_menu_tool(); };
        };
#ifndef NDEBUG
        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
#endif
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
    toolkit::installUePath(CoreSet::getSet().gettUe4Setting().Path() / "Engine");

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
void main_windows::main_menu_windows() {
  dear::MenuItem(u8"后台任务", p_long_task_show.get());
}
void main_windows::main_menu_edit() {
  if (dear::MenuItem(u8"导出fbx"))
    p_long_task->set_tool_widget(new_object<comm_export_fbx>());
  if (dear::MenuItem(u8"解算布料"))
    p_long_task->set_tool_widget(new_object<comm_qcloth_sim>());
  //  if (dear::MenuItem(u8"创建视频"))
  //    p_long_task->set_tool_widget(new_object<comm_export_fbx>());
  //  if (dear::MenuItem(u8"导入ue文件"))
  //    p_long_task->set_tool_widget(new_object<comm_export_fbx>());
  //  if (dear::MenuItem(u8"创建ue关卡"))
  //    p_long_task->set_tool_widget(new_object<comm_export_fbx>());
}

}  // namespace doodle
