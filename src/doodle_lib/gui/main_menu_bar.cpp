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
#include <gui/widgets/project_widget.h>
#include <gui/widgets/assets_filter_widget.h>
#include <gui/setting_windows.h>
#include <gui/get_input_dialog.h>
#include <gui/widgets/long_time_tasks_widget.h>
#include <gui/widgets/edit_widgets.h>
#include <gui/widgets/opencv_player_widget.h>
#include <gui/widgets/assets_file_widgets.h>
#include <gui/widgets/csv_export_widgets.h>
#include <gui/widgets/ue4_widget.h>

#include <doodle_lib/metadata/project.h>
#include <doodle_lib/metadata/metadata.h>
#include <doodle_lib/long_task/database_task.h>
#include <doodle_lib/thread_pool/process_message.h>
#include <doodle_lib/gui/widgets/project_edit.h>
#include <doodle_lib/core/core_sig.h>
#include <doodle_lib/core/init_register.h>
#include <doodle_lib/gui/gui_ref/base_window.h>

namespace doodle {
class main_menu_bar::impl {
 public:
  bool p_debug_show{false};
  bool p_style_show{false};
  bool p_about_show{false};
  std::map<std::string, gui::windows_proc::warp_proc_ptr> windows_;
};

main_menu_bar::main_menu_bar()
    : p_i(std::make_unique<impl>()) {
}

main_menu_bar::~main_menu_bar() = default;

void main_menu_bar::menu_file() {
  if (dear::MenuItem("新项目"s)) {
    auto k_h = make_handle();
    k_h.emplace<project>();
    auto l_ptr = std::make_shared<FSys::path>();
    g_main_loop()
        .attach<file_dialog>(file_dialog::dialog_args{l_ptr}
                                 .set_title("选择目录"s)
                                 .set_use_dir())
        .then<one_process_t>([=]() {
          k_h.patch<project>([=](project &in) {
            in.p_path = *l_ptr;
          });
        })
        .then<get_input_project_dialog>(k_h)
        .then<one_process_t>([=]() {
          core::client l_c{};
          l_c.new_project(k_h);
        });
  }
  if (dear::MenuItem("打开项目"s)) {
    auto l_ptr = std::make_shared<FSys::path>();
    g_main_loop()
        .attach<file_dialog>(file_dialog::dialog_args{l_ptr}
                                 .set_title("选择文件")
                                 .add_filter(std::string{doodle_config::doodle_db_name}))
        .then<one_process_t>([=]() {
          core::client l_c{};
          l_c.open_project(*l_ptr);
        });
  }
  dear::Menu{"最近的项目"} && []() {
    auto &k_list = core_set::getSet().project_root;
    for (int l_i = 0; l_i < k_list.size(); ++l_i) {
      auto &l_p = k_list[l_i];
      if (!l_p.empty())
        if (dear::MenuItem(fmt::format("{0}##{1}", l_p.generic_string(), l_i))) {
          core::client l_c{};
          l_c.open_project(l_p);
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
  for (auto &l_win : p_i->windows_) {
    if (dear::MenuItem(l_win.first.c_str(), l_win.second->is_show())) {
      if (!l_win.second->is_show()) {
        for (auto &&l_item : init_register::instance().get_derived_class<gui::window_panel>()) {
          if (l_item.prop("name"_hs).value() == l_win.first) {
            auto l_win_obj                        = l_item.construct();
            auto l_win_ptr                        = l_win_obj.try_cast<gui::base_window>();
            auto l_w                              = g_main_loop().attach<gui::windows_proc>(l_win_ptr, std::move(l_win_obj)).get<gui::windows_proc>();
            p_i->windows_[l_w->windows_->title()] = l_w->warp_proc_;
          }
        }
      } else {
        l_win.second->close();
      }
    }
  }
}

void main_menu_bar::menu_tool() {
  if (dear::MenuItem("安装maya插件"))
    toolkit::installMayaPath();
  if (dear::MenuItem("安装ue4插件"))
    toolkit::installUePath(core_set::getSet().ue4_path / "Engine");
  if (dear::MenuItem("安装ue4项目插件")) {
    auto l_ptr = std::make_shared<FSys::path>();
    g_main_loop()
        .attach<file_dialog>(file_dialog::dialog_args{l_ptr}
                                 .set_title("select_ue_project"s)
                                 .add_filter(".uproject"))
        .then<one_process_t>([=]() {
          toolkit::installUePath(*l_ptr);
        });
  }
  if (dear::MenuItem("删除ue4缓存"))
    toolkit::deleteUeCache();
  if (dear::MenuItem("修改ue4缓存位置"))
    toolkit::modifyUeCachePath();
}

void main_menu_bar::init() {
  g_reg()->set<main_menu_bar &>(*this);
  for (auto &&l_item : init_register::instance().get_derived_class<gui::window_panel>()) {
    if (auto l_win = l_item.construct(); l_win) {
      auto l_win_ptr = l_win.try_cast<gui::base_window>();
      auto l_w       = g_main_loop().attach<gui::windows_proc>(l_win_ptr, std::move(l_win)).get<gui::windows_proc>();
      p_i->windows_.emplace(l_w->windows_->title(), l_w->warp_proc_);
    }
  }
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
