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
#include <doodle_lib/gui/widgets/opencv_player_widget.h>
#include <doodle_lib/gui/widgets/project_widget.h>
#include <doodle_lib/gui/widgets/tool_box_widget.h>
#include <doodle_lib/lib_warp/imgui_warp.h>
#include <doodle_lib/toolkit/toolkit.h>
namespace doodle {

template <class T>
T *main_windows::create_windwos(bool is_show) {
  auto t = new_object<windows_warp<T>>(true);
  p_list_windwos.push_back(t);
  return win_cast<T>(t).get();
}

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
      p_ass_(nullptr),
      p_prj_(nullptr),
      p_setting_(nullptr),
      p_attr_(nullptr),
      p_long_task_(nullptr),
      p_edit_windows_(nullptr),
      p_tool_box_(nullptr),
      p_opencv_(nullptr),
      p_list_windwos() {
  p_class_name    = "main_doodle";
  p_prj_          = create_windwos<project_widget>(true);
  p_ass_          = create_windwos<assets_widget>(true);
  p_attr_         = create_windwos<assets_file_widgets>(true);
  p_setting_      = create_windwos<setting_windows>(false);
  p_long_task_    = create_windwos<long_time_tasks_widget>(true);
  p_edit_windows_ = create_windwos<edit_widgets>(true);
  p_tool_box_     = create_windwos<tool_box_widget>(true);
  p_opencv_       = create_windwos<opencv_player_widget>(false);

  for (auto &i : p_list_windwos) {
    i->load_show();
  }

  p_prj_->select_change.connect([this](auto in) {
    p_ass_->set_metadata(in);
    p_edit_windows_->set_factort(p_prj_->get_factory());
  });
  p_ass_->select_change.connect([this](auto in) {
    p_edit_windows_->set_factort(p_ass_->get_factory());
  });
  p_attr_->select_change.connect(
      [this](auto in) {
        p_edit_windows_->set_factort(p_attr_->get_factory());
      });

  p_edit_windows_->set_factort(p_prj_->get_factory());
}
void main_windows::frame_render() {
  for (auto &i : p_list_windwos) {
    i->frame_render();
  }

  if (*p_debug_show) imgui::ShowMetricsWindow(p_debug_show.get());
  if (*p_about_show) imgui::ShowAboutWindow(p_about_show.get());
  if (*p_style_show) {
    dear::Begin{"界面样式编辑", p_style_show.get()} && []() {
      imgui::ShowStyleEditor();
    };
  }
  dear::MainMenuBar{} &&
      [this]() {
        dear::Menu{"文件"} && [this]() { this->main_menu_file(); };
        dear::Menu{"窗口"} && [this]() { this->main_menu_windows(); };
        dear::Menu{"编辑"} && [this]() { this->main_menu_edit(); };
        dear::Menu{"工具"} && [this]() { this->main_menu_tool(); };
#ifndef NDEBUG
        ImGui::Text("%.3f ms/%.1f FPS", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
#endif
        //        imgui::SameLine();
      };
}
void main_windows::main_menu_file() {
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
    doodle_app::Get()->main_loop.connect_extended([](const doodle_app::connection &in) {
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
  for (auto &i : p_list_windwos) {
    dear::MenuItem(i->get_class_name(), i->p_show.get());
  }
}

void main_windows::main_menu_edit() {
  auto k_task = p_tool_box_;
  if (dear::MenuItem(u8"maya 工具"))
    k_task->set_tool_widget(new_object<comm_maya_tool>());
  if (dear::MenuItem(u8"创建视频"))
    k_task->set_tool_widget(new_object<comm_create_video>());
  if (dear::MenuItem(u8"ue工具"))
    k_task->set_tool_widget(new_object<comm_import_ue_files>());
}
main_windows::~main_windows() {
  for (auto &i : p_list_windwos) {
    i->save_show();
  }
  core_set_init{}.write_file();
  for (auto &i : p_list_windwos) {
    *(i->p_show) = false;
  }
}
}  // namespace doodle
