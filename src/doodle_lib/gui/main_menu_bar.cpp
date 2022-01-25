//
// Created by TD on 2022/1/14.
//

#include "main_menu_bar.h"
#include <lib_warp/imgui_warp.h>
#include <doodle_lib/app/app.h>

#include <doodle_lib/core/core_set.h>
#include <doodle_lib/client/client.h>
#include <doodle_lib/long_task/process_pool.h>
#include <gui/open_file_dialog.h>
#include <toolkit/toolkit.h>
#include <gui/action/command_tool.h>
#include <gui/action/command_video.h>
#include <gui/action/command_meta.h>
#include <gui/widgets/project_widget.h>
#include <gui/widgets/assets_widget.h>
#include <gui/setting_windows.h>
#include <gui/get_input_dialog.h>
#include <gui/widgets/long_time_tasks_widget.h>
#include <gui/widgets/edit_widgets.h>
#include <gui/widgets/tool_box_widget.h>
#include <gui/widgets/opencv_player_widget.h>
#include <gui/widgets/assets_file_widgets.h>
#include <doodle_lib/metadata/project.h>
#include <doodle_lib/metadata/metadata.h>

namespace doodle {
class main_menu_bar::impl {
 public:
  bool p_debug_show{false};
  bool p_style_show{false};
  bool p_about_show{false};
};

main_menu_bar::main_menu_bar()
    : p_i(std::make_unique<impl>()) {
}

main_menu_bar::~main_menu_bar() = default;

void main_menu_bar::menu_file() {
  if (dear::MenuItem("新项目"s)) {
    auto k_h = make_handle();
    k_h.emplace<project>();
    g_main_loop().attach<file_dialog>(
                     [=](const FSys::path &in_path) {
                       core::client{}.add_project(in_path);
                       g_reg()->set<project>(in_path, "none");
                       k_h.patch<project>([&](project &in) {
                         in.p_path = in_path;
                       });
                       core_set::getSet().add_recent_project(in_path);
                     },
                     "选择目录"s)
        .then<get_input_project_dialog>(k_h)
        .then<one_process_t>([=]() {
          k_h.emplace<database>();
          k_h.patch<database>(database::save{});
          g_reg()->set<project>(k_h.get<project>());
          core_set::getSet().add_recent_project(k_h.get<project>().get_path());
        });
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
        if (dear::MenuItem(fmt::format("{0}##{1}", l_p.generic_string(), l_i))) {
          g_reg()->set<project>(l_p, "temp_project");
          core::client{}.open_project(l_p);
        }
    }
  };

  ImGui::Separator();
  dear::MenuItem("调试"s, &p_i->p_debug_show);
  dear::MenuItem("样式设置"s, &p_i->p_style_show);
  dear::MenuItem("关于"s, &p_i->p_about_show);
  ImGui::Separator();
  if (dear::MenuItem(u8"退出")) {
    app::Get().stop();
  }
}

void main_menu_bar::menu_windows() {
  //  auto k_prj = g_reg()->try_ctx<project_widget>();

  {
    auto k_win = g_reg()->try_ctx<base_window<project_widget>>();
    if (dear::MenuItem(project_widget::name.data(), k_win ? &(k_win->show) : nullptr))
      make_windows<project_widget>();
  }
  {
    auto k_win = g_reg()->try_ctx<base_window<assets_widget>>();
    if (dear::MenuItem(assets_widget::name.data(), k_win ? &(k_win->show) : nullptr))
      make_windows<assets_widget>();
  }
  {
    auto k_win = g_reg()->try_ctx<base_window<assets_file_widgets>>();
    if (dear::MenuItem(assets_file_widgets::name.data(), k_win ? &(k_win->show) : nullptr))
      make_windows<assets_file_widgets>();
  }
  {
    auto k_win = g_reg()->try_ctx<base_window<setting_windows>>();
    if (dear::MenuItem(setting_windows::name.data(), k_win ? &(k_win->show) : nullptr))
      make_windows<setting_windows>();
  }
  {
    auto k_win = g_reg()->try_ctx<base_window<long_time_tasks_widget>>();
    if (dear::MenuItem(long_time_tasks_widget::name.data(), k_win ? &(k_win->show) : nullptr))
      make_windows<long_time_tasks_widget>();
  }
  {
    auto k_win = g_reg()->try_ctx<base_window<edit_widgets>>();
    if (dear::MenuItem(edit_widgets::name.data(), k_win ? &(k_win->show) : nullptr))
      make_windows<edit_widgets>();
  }
  {
    auto k_win = g_reg()->try_ctx<base_window<tool_box_widget>>();
    if (dear::MenuItem(tool_box_widget::name.data(), k_win ? &(k_win->show) : nullptr))
      make_windows<tool_box_widget>();
  }
  {
    auto k_win = g_reg()->try_ctx<base_window<opencv_player_widget>>();
    if (dear::MenuItem(opencv_player_widget::name.data(), k_win ? &(k_win->show) : nullptr))
      make_windows<opencv_player_widget>();
  }
}
void main_menu_bar::menu_edit() {
  if (dear::MenuItem(u8"maya 工具"))
    make_windows<comm_maya_tool>();
  if (dear::MenuItem(u8"创建视频"))
    make_windows<comm_create_video>();
  if (dear::MenuItem(u8"ue工具"))
    make_windows<comm_import_ue_files>();
}
void main_menu_bar::menu_tool() {
  if (dear::MenuItem("安装maya插件"))
    toolkit::installMayaPath();
  if (dear::MenuItem("安装ue4插件"))
    toolkit::installUePath(core_set::getSet().get_ue4_setting().get_path() / "Engine");
  if (dear::MenuItem("安装ue4项目插件")) {
    g_main_loop().attach<file_dialog>(
        [](const FSys::path &in_path) {
          toolkit::installUePath(in_path.parent_path());
        },
        "select_ue_project",
        string_list{".uproject"});
  }
  if (dear::MenuItem("删除ue4缓存"))
    toolkit::deleteUeCache();
  if (dear::MenuItem("修改ue4缓存位置"))
    toolkit::modifyUeCachePath();
}

void main_menu_bar::init() {
  g_reg()->set<main_menu_bar &>(*this);
  auto k_windows_setting = core_set::getSet().widget_show;

  if (k_windows_setting.find(std::string{project_widget::name}) != k_windows_setting.end())
    if (k_windows_setting[std::string{project_widget::name}])
      make_windows<project_widget>();
  if (k_windows_setting.find(std::string{assets_widget::name}) != k_windows_setting.end())
    if (k_windows_setting[std::string{assets_widget::name}])
      make_windows<assets_widget>();
  if (k_windows_setting.find(std::string{assets_file_widgets::name}) != k_windows_setting.end())
    if (k_windows_setting[std::string{assets_file_widgets::name}])
      make_windows<assets_file_widgets>();
  if (k_windows_setting.find(std::string{setting_windows::name}) != k_windows_setting.end())
    if (k_windows_setting[std::string{setting_windows::name}])
      make_windows<setting_windows>();
  if (k_windows_setting.find(std::string{long_time_tasks_widget::name}) != k_windows_setting.end())
    if (k_windows_setting[std::string{long_time_tasks_widget::name}])
      make_windows<long_time_tasks_widget>();
  if (k_windows_setting.find(std::string{edit_widgets::name}) != k_windows_setting.end())
    if (k_windows_setting[std::string{edit_widgets::name}])
      make_windows<edit_widgets>();
  if (k_windows_setting.find(std::string{tool_box_widget::name}) != k_windows_setting.end())
    if (k_windows_setting[std::string{tool_box_widget::name}])
      make_windows<tool_box_widget>();
  if (k_windows_setting.find(std::string{opencv_player_widget::name}) != k_windows_setting.end())
    if (k_windows_setting[std::string{opencv_player_widget::name}])
      make_windows<opencv_player_widget>();
}
void main_menu_bar::succeeded() {
}
void main_menu_bar::failed() {
}
void main_menu_bar::aborted() {
}
void main_menu_bar::update(
    std::chrono::duration<std::chrono::system_clock::rep, std::chrono::system_clock::period>,
    void *data) {
  dear::MainMenuBar{} && [this]() {
    dear::Menu{"文件"} && [this]() { this->menu_file(); };
    dear::Menu{"窗口"} && [this]() { this->menu_windows(); };
    dear::Menu{"编辑"} && [this]() { this->menu_edit(); };
    dear::Menu{"工具"} && [this]() { this->menu_tool(); };
#ifndef NDEBUG
    ImGui::Text("%.3f ms/%.1f FPS", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
#endif
  };
}

}  // namespace doodle
