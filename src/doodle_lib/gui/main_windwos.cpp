//
// Created by TD on 2021/9/15.
//

#include "main_windwos.h"

#include <doodle_lib/Exception/exception.h>
#include <doodle_lib/doodle_app.h>
#include <doodle_lib/gui/action/command_tool.h>
#include <doodle_lib/gui/setting_windows.h>
#include <doodle_lib/gui/widgets/assets_file_widgets.h>
#include <doodle_lib/gui/widgets/assets_widget.h>
#include <doodle_lib/gui/widgets/edit_widgets.h>
#include <doodle_lib/gui/widgets/long_time_tasks_widget.h>
#include <doodle_lib/gui/widgets/project_widget.h>
#include <doodle_lib/gui/widgets/tool_box_widget.h>
#include <doodle_lib/lib_warp/imgui_warp.h>
#include <doodle_lib/toolkit/toolkit.h>
namespace doodle {
main_windows::main_windows()
    : p_debug_show(new_object<bool>(false)),
      p_about_show(new_object<bool>(false)),
      p_style_show(new_object<bool>(false)),
      p_title(fmt::format(
          u8"doodle {}.{}.{}.{}",
          Doodle_VERSION_MAJOR,
          Doodle_VERSION_MINOR,
          Doodle_VERSION_PATCH,
          Doodle_VERSION_TWEAK)),
      p_ass(new_object<assets_widget>()),
      p_prj(new_object<project_widget>()),

      p_setting(new_object<windows_warp<setting_windows>>()),
      p_attr(new_object<windows_warp<assets_file_widgets>>(true)),
      p_long_task(new_object<windows_warp<long_time_tasks_widget>>(true)),
      p_edit_windows(new_object<windows_warp<edit_widgets>>(true)),
      p_tool_box(new_object<windows_warp<tool_box_widget>>(true)) {
  p_prj->select_change.connect([this](auto in) {
    p_ass->set_metadata(in);
    win_cast<edit_widgets>(p_edit_windows)->set_factort(p_prj->get_factory());
  });
  p_ass->select_change.connect([this](auto in) {
    win_cast<assets_file_widgets>(p_attr)->set_metadata(in);
    win_cast<edit_widgets>(p_edit_windows)->set_factort(p_ass->get_factory());
  });
  win_cast<assets_file_widgets>(p_attr)->select_change.connect(
      [this](auto in) {
        win_cast<edit_widgets>(
            p_edit_windows)
            ->set_factort(
                win_cast<assets_file_widgets>(p_attr)->get_factory());
      });

  win_cast<edit_widgets>(p_edit_windows)->set_factort(p_prj->get_factory());
}
void main_windows::frame_render() {
  p_setting->frame_render();
  p_long_task->frame_render();
  p_attr->frame_render();
  p_edit_windows->frame_render();
  p_tool_box->frame_render();

  if (*p_debug_show) imgui::ShowMetricsWindow(p_debug_show.get());
  if (*p_about_show) imgui::ShowAboutWindow(p_about_show.get());
  if (*p_style_show) {
    dear::Begin{"界面样式编辑", p_style_show.get()} && []() {
      imgui::ShowStyleEditor();
    };
  }

  dear::Begin{
      p_title.c_str(),
      nullptr,
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
      };
}
void main_windows::main_menu_file() {
  dear::MenuItem(
      win_cast<setting_windows>(p_setting)->get_class_name().c_str(),
      win_warp_cast<setting_windows>(p_setting)->p_show.get());

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
    toolkit::installUePath(core_set::getSet().get_ue4_setting().get_path() / "Engine");

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
  dear::MenuItem(
      win_cast<assets_file_widgets>(p_attr)->get_class_name().c_str(),
      win_warp_cast<assets_file_widgets>(p_attr)->p_show.get());
  dear::MenuItem(
      win_cast<long_time_tasks_widget>(p_long_task)->get_class_name().c_str(),
      win_warp_cast<long_time_tasks_widget>(p_long_task)->p_show.get());
  dear::MenuItem(
      win_cast<edit_widgets>(p_edit_windows)->get_class_name().c_str(),
      win_warp_cast<edit_widgets>(p_edit_windows)->p_show.get());
}

void main_windows::main_menu_edit() {
  auto k_task = win_cast<tool_box_widget>(p_tool_box);
  if (dear::MenuItem(u8"导出fbx"))
    k_task->set_tool_widget(new_object<comm_export_fbx>());
  if (dear::MenuItem(u8"解算布料"))
    k_task->set_tool_widget(new_object<comm_qcloth_sim>());
  if (dear::MenuItem(u8"创建视频"))
    k_task->set_tool_widget(new_object<comm_create_video>());
  if (dear::MenuItem(u8"ue工具"))
    k_task->set_tool_widget(new_object<comm_import_ue_files>());
}

}  // namespace doodle
