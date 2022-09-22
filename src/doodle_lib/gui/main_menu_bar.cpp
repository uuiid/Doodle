//
// Created by TD on 2022/1/14.
//

#include "main_menu_bar.h"
#include <lib_warp/imgui_warp.h>
#include <doodle_lib/app/app.h>

#include <gui/open_file_dialog.h>
#include <toolkit/toolkit.h>
#include <gui/setting_windows.h>
#include <gui/widgets/project_edit.h>
#include <gui/get_input_dialog.h>
#include <gui/gui_ref/ref_base.h>
#include <doodle_core/core/core_sig.h>
#include <doodle_lib/gui/gui_ref/layout_window.h>

#include <doodle_core/database_task/sqlite_client.h>
#include <doodle_lib/gui/gui_ref/layout_window.h>
#include <doodle_lib/gui/widgets/edit_widget.h>
#include <doodle_lib/gui/widgets/assets_filter_widget.h>
#include <doodle_lib/gui/widgets/csv_export_widgets.h>
#include <doodle_lib/gui/widgets/maya_tool.h>
#include <doodle_lib/gui/widgets/create_video.h>
#include <doodle_lib/gui/widgets/ue4_widget.h>
#include <doodle_lib/gui/widgets/extract_subtitles_widgets.h>
#include <doodle_lib/gui/widgets/subtitle_processing.h>
#include <doodle_lib/gui/widgets/assets_file_widgets.h>
#include <doodle_lib/gui/widgets/long_time_tasks_widget.h>
#include <doodle_lib/gui/widgets/time_sequencer_widget.h>
#include <doodle_lib/gui/widgets/all_user_view_widget.h>

namespace doodle::gui {
namespace main_menu_bar_ns {
void to_json(nlohmann::json &j, const layout_data &p) {
  j["imgui_data"] = p.imgui_data;
  j["name"]       = p.name;
}
void from_json(const nlohmann::json &j, layout_data &p) {
  j["imgui_data"].get_to(p.imgui_data);
  j["name"].get_to(p.name);
}
bool layout_data::operator==(const layout_data &in_rhs) const {
  return name == in_rhs.name;
}
bool layout_data::operator!=(const layout_data &in_rhs) const {
  return !(in_rhs == *this);
}
bool layout_data::operator==(const std::string &in_rhs) const {
  return name == in_rhs;
}
bool layout_data::operator!=(const std::string &in_rhs) const {
  return !(*this == in_rhs);
}
}  // namespace main_menu_bar_ns

class main_menu_bar::impl {
 public:
  bool p_debug_show{false};
  bool p_style_show{false};
  bool p_about_show{false};

  gui::gui_cache<std::string> layout_name_{"##name"s, "layout_name"s};
  gui::gui_cache_name_id button_save_layout_name_{"保存"};
  gui::gui_cache_name_id button_delete_layout_name_{"删除当前布局"};

  std::vector<main_menu_bar_ns::layout_data> layout_list;
};

main_menu_bar::main_menu_bar()
    : p_i(std::make_unique<impl>()) {
}
main_menu_bar::~main_menu_bar() = default;

void main_menu_bar::menu_file() {
  if (dear::MenuItem("创建项目"s)) {
    auto l_ptr = std::make_shared<FSys::path>();

    make_process_adapter<file_dialog>(
        strand_gui{g_io_context().get_executor()},
        file_dialog::dialog_args{l_ptr}
            .set_title("选择目录"s)
            .set_use_dir()
    )
        .next<get_input_project_dialog>(l_ptr)();
  }
  if (dear::MenuItem("打开项目"s)) {
    auto l_ptr = std::make_shared<FSys::path>();
    make_process_adapter<file_dialog>(strand_gui{g_io_context().get_executor()}, file_dialog::dialog_args{l_ptr}.set_title("选择文件").add_filter(std::string{doodle_config::doodle_db_name}))
        .next([=]() {
          std::make_shared<database_n::sqlite_file>()->async_open(*l_ptr, [l_ptr](auto) {
            DOODLE_LOG_INFO("打开项目 {}", *l_ptr);
          });
        })();
  }
  dear::Menu{"最近的项目"} && []() {
    auto &k_list = core_set::get_set().project_root;
    for (int l_i = 0; l_i < k_list.size(); ++l_i) {
      auto &l_p = k_list[l_i];
      if (!l_p.empty())
        if (dear::MenuItem(fmt::format("{0}##{1}", l_p.generic_string(), l_i))) {
          std::make_shared<database_n::sqlite_file>()->async_open(l_p, [l_p](auto) {
            DOODLE_LOG_INFO("打开项目 {}", l_p);
          });
        }
    }
  };

  ImGui::Separator();

  if (dear::MenuItem("保存"s, "Ctrl+S"))
    g_reg()->ctx().at<core_sig>().save();

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
  if (dear::MenuItem(setting_windows::name.data()))
    make_handle().emplace<gui_windows>(setting_windows{});
  if (dear::MenuItem(project_edit::name.data()))
    make_handle().emplace<gui_windows>(project_edit{});
  if (dear::MenuItem(edit_widgets::name.data()))
    make_handle().emplace<gui_windows>(edit_widgets{});
  if (dear::MenuItem(assets_filter_widget::name.data()))
    make_handle().emplace<gui_windows>(assets_filter_widget{});
  if (dear::MenuItem(csv_export_widgets::name.data()))
    make_handle().emplace<gui_windows>(csv_export_widgets{});
  if (dear::MenuItem(maya_tool::name.data()))
    make_handle().emplace<gui_windows>(maya_tool{});
  if (dear::MenuItem(create_video::name.data()))
    make_handle().emplace<gui_windows>(create_video{});
  if (dear::MenuItem(extract_subtitles_widgets::name.data()))
    make_handle().emplace<gui_windows>(extract_subtitles_widgets{});
  if (dear::MenuItem(subtitle_processing::name.data()))
    make_handle().emplace<gui_windows>(subtitle_processing{});
  if (dear::MenuItem(assets_file_widgets::name.data()))
    make_handle().emplace<gui_windows>(assets_file_widgets{});
  if (dear::MenuItem(long_time_tasks_widget::name.data()))
    make_handle().emplace<gui_windows>(long_time_tasks_widget{});
  if (dear::MenuItem(time_sequencer_widget::name.data()))
    make_handle().emplace<gui_windows>(time_sequencer_widget{});
  if (dear::MenuItem(all_user_view_widget::name.data()))
    make_handle().emplace<gui_windows>(all_user_view_widget{});
}

void main_menu_bar::menu_tool() {
  if (dear::MenuItem("安装maya插件"))
    toolkit::installMayaPath();
  if (dear::MenuItem("安装ue4插件"))
    toolkit::installUePath(core_set::get_set().ue4_path / "Engine");
  if (dear::MenuItem("安装ue4项目插件")) {
    auto l_ptr = std::make_shared<FSys::path>();
    make_process_adapter<file_dialog>(strand_gui{g_io_context().get_executor()}, file_dialog::dialog_args{l_ptr}.set_title("select_ue_project"s).add_filter(".uproject"))
        .next([=]() {
          toolkit::installUePath(*l_ptr);
        })();
  }
  if (dear::MenuItem("删除ue4缓存"))
    toolkit::deleteUeCache();
  if (dear::MenuItem("修改ue4缓存位置"))
    toolkit::modifyUeCachePath();
  if (dear::MenuItem("安装houdini 19.0插件"))
    toolkit::install_houdini_plug();
}

bool main_menu_bar::tick() {
  dear::MainMenuBar{} && [this]() {
    dear::Menu{"文件"} && [this]() { this->menu_file(); };
    dear::Menu{"窗口"} && [this]() { this->menu_windows(); };
    dear::Menu{"编辑"} && [this]() { this->menu_edit(); };
    dear::Menu{"工具"} && [this]() { this->menu_tool(); };
#ifndef NDEBUG
    ImGui::Text("%.3f ms/%.1f FPS", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
#endif
  };
  return false;
}
void main_menu_bar::menu_edit() {
  dear::Menu{"布局"} && [this]() { this->menu_layout(); };
}
void main_menu_bar::menu_layout() {
  //  ImGui::InputText(*p_i->layout_name_.gui_name, &p_i->layout_name_.data);
  //  ImGui::SameLine();
  //  if (ImGui::Button(*p_i->button_save_layout_name_))

  //
  //  for (auto &&i : p_i->layout_list) {
  //    if (ImGui::MenuItem(i.name.c_str())) {

  //    };
  //  }
}
main_menu_bar::main_menu_bar(const main_menu_bar &in) noexcept
    : p_i(std::make_unique<impl>(*in.p_i)) {
}
main_menu_bar::main_menu_bar(main_menu_bar &&in) noexcept {
  p_i = std::move(in.p_i);
}
main_menu_bar &main_menu_bar::operator=(const main_menu_bar &in) noexcept {
  *p_i = *p_i;
  return *this;
}
main_menu_bar &main_menu_bar::operator=(main_menu_bar &&in) noexcept {
  p_i = std::move(in.p_i);
  return *this;
}

}  // namespace doodle::gui
