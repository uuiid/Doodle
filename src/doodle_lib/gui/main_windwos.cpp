//
// Created by TD on 2021/9/15.
//

#include "main_windwos.h"

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
#include <doodle_lib/toolkit/toolkit.h>
#include <doodle_lib/long_task/process_pool.h>
#include <doodle_lib/core/open_file_dialog.h>
#include <doodle_lib/client/client.h>

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
    p_tool_box_->set_tool_widget(nullptr);
  });
  p_ass_->select_change.connect([this](auto in) {
  });
  p_attr_->select_change.connect(
      [this](auto in) {
      });
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
  if (dear::MenuItem("新项目"s)) {
    g_main_loop().attach<file_dialog>(
        [](const FSys::path &in_path) {
          core::client{}.add_project(in_path);
          g_reg()->set<project>(in_path, "none");
          core_set::getSet().add_recent_project(in_path);
        },
        "选择目录"s);
  }
  if (dear::MenuItem("打开项目"s)) {
    g_main_loop().attach<file_dialog>(
        [](const FSys::path &in_path) {
          g_reg()->set<project>(in_path, "temp_project");
          core::client{}.open_project(in_path);
          core_set::getSet().add_recent_project(in_path);
        },
        "选择目录"s);
  }
  dear::Menu{"最近的项目"} && []() {
    auto &k_list = core_set::getSet().project_root;
    for (int l_i = 0; l_i < k_list.size(); ++l_i) {
      auto &l_p = k_list[l_i];
      if (!l_p.empty())
        if (dear::MenuItem(fmt::format("{0}##{0}{1}", l_p.generic_string(), l_i))) {
          g_reg()->set<project>(l_p, "temp_project");
          core::client{}.open_project(l_p);
        }
    }
  };

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
    g_main_loop().attach<file_dialog>(
        [](const FSys::path &in_path) {
          toolkit::installUePath(in_path);
        },
        "select_ue_project",
        string_list{".uproject"});
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
  p_list_windwos.clear();
  core_set_init{}.write_file();
}
}  // namespace doodle
