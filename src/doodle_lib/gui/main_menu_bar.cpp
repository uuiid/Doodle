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
#include <gui/widgets/project_widget.h>
#include <gui/widgets/assets_widget.h>
#include <gui/setting_windows.h>
#include <gui/get_input_dialog.h>
#include <gui/widgets/long_time_tasks_widget.h>
#include <gui/widgets/edit_widgets.h>
#include <gui/widgets/opencv_player_widget.h>
#include <gui/widgets/assets_file_widgets.h>
#include <gui/widgets/csv_export_widgets.h>

#include <doodle_lib/metadata/project.h>
#include <doodle_lib/metadata/metadata.h>
#include <doodle_lib/long_task/database_task.h>
#include <doodle_lib/thread_pool/long_term.h>
#include <doodle_lib/gui/widgets/project_edit.h>
#include <doodle_lib/core/core_sig.h>

namespace doodle {
class main_menu_bar::impl {
 public:
  bool p_debug_show{false};
  bool p_style_show{false};
  bool p_about_show{false};

  template <typename Widget_T>
  void _make_widget_() {
    // using win  = typename base_window<Widget_T>;

    auto k_win = g_reg()->template try_ctx<base_window<Widget_T>>();
    if (dear::MenuItem(Widget_T::name.data(), k_win ? &(k_win->show) : nullptr)) {
      make_windows<Widget_T>();
      core_set_init{}.write_file();
    }
  };

  template <typename... Args>
  void make_widget() {
    (this->_make_widget_<Args>(), ...);
  }

  template <typename Widget_T>
  void _find_show_() {
    auto &&k_windows_setting = core_set::getSet().widget_show;

    if (k_windows_setting.find(std::string{Widget_T::name}) != k_windows_setting.end())
      if (k_windows_setting[std::string{Widget_T::name}])
        make_windows<Widget_T>();
  }

  template <typename... Args>
  void find_show() {
    (this->_find_show_<Args>(), ...);
  }
};

main_menu_bar::main_menu_bar()
    : p_i(std::make_unique<impl>()) {
}

main_menu_bar::~main_menu_bar() = default;

void main_menu_bar::menu_file() {
  if (dear::MenuItem("新项目"s)) {
    auto k_h = make_handle();
    k_h.emplace<project>();

    g_main_loop()
        .attach<file_dialog>(
            [=](const FSys::path &in_path) {
              k_h.patch<project>([&](project &in) {
                in.p_path = in_path;
              });
            },
            "选择目录"s)
        .then<get_input_project_dialog>(k_h)
        .then<one_process_t>([=]() {
          core::client{}.new_project(k_h);
        });
  }
  if (dear::MenuItem("打开项目"s)) {
    g_main_loop().attach<file_dialog>(
        [](const FSys::path &in_path) {
          core::client{}.open_project(in_path);
        },
        "选择文件"s,
        std::vector<std::string>{std::string{doodle_config::doodle_db_name}});
  }
  dear::Menu{"最近的项目"} && []() {
    auto &k_list = core_set::getSet().project_root;
    for (int l_i = 0; l_i < k_list.size(); ++l_i) {
      auto &l_p = k_list[l_i];
      if (!l_p.empty())
        if (dear::MenuItem(fmt::format("{0}##{1}", l_p.generic_string(), l_i))) {
          core::client{}.open_project(l_p);
        }
    }
  };

  ImGui::Separator();

  if (dear::MenuItem("保存"s, "Ctrl+S"))
    g_reg()->ctx<core_sig>().save();

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
  this->p_i->make_widget<project_widget,
                         assets_widget,
                         assets_file_widgets,
                         setting_windows,
                         long_time_tasks_widget,
                         edit_widgets,
                         opencv_player_widget,
                         gui::csv_export_widgets,
                         project_edit>();
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

  p_i->find_show<project_widget,
                 assets_widget,
                 assets_file_widgets,
                 setting_windows,
                 long_time_tasks_widget,
                 edit_widgets,
                 gui::csv_export_widgets,
                 opencv_player_widget>();
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
