//
// Created by TD on 2022/1/14.
//

#include "main_menu_bar.h"

#include <doodle_core/core/core_sig.h>
#include <doodle_core/database_task/sqlite_client.h>

#include <doodle_app/app/app_command.h>
#include <doodle_app/gui/base/ref_base.h>
#include <doodle_app/gui/get_input_dialog.h>
#include <doodle_app/gui/open_file_dialog.h>

#include "base/base_window.h"
#include <lib_warp/imgui_warp.h>

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
bool layout_data::operator==(const layout_data &in_rhs) const { return name == in_rhs.name; }
bool layout_data::operator!=(const layout_data &in_rhs) const { return !(in_rhs == *this); }
bool layout_data::operator==(const std::string &in_rhs) const { return name == in_rhs; }
bool layout_data::operator!=(const std::string &in_rhs) const { return !(*this == in_rhs); }
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

main_menu_bar::main_menu_bar() : p_i(std::make_unique<impl>()) {}
main_menu_bar::~main_menu_bar() = default;

void main_menu_bar::menu_file() {
  if (dear::MenuItem("创建项目"s)) {
    g_windows_manage().create_windows_arg(
        windows_init_arg{}.create<create_project_dialog>().set_render_type<dear::Popup>()
    );
  }
  if (dear::MenuItem("打开项目"s)) {
    g_windows_manage().create_windows_arg(
        windows_init_arg{}
            .create<file_dialog>(file_dialog::dialog_args{}.async_read([](const FSys::path &in) mutable {
              doodle_lib::Get().ctx().at<database_n::file_translator_ptr>()->async_open(in, [in](auto) {
                DOODLE_LOG_INFO("打开项目 {}", in);
              });
            }))
            .set_title("打开项目")
            .set_render_type<dear::Popup>()

    );
  }
  dear::Menu{"最近的项目"} && []() {
    auto &k_list = core_set::get_set().project_root;
    for (int l_i = 0; l_i < k_list.size(); ++l_i) {
      auto &l_p = k_list[l_i];
      if (!l_p.empty())
        if (dear::MenuItem(fmt::format("{0}##{1}", l_p.generic_string(), l_i))) {
          doodle_lib::Get().ctx().at<database_n::file_translator_ptr>()->async_open(l_p, [l_p](auto) {
            DOODLE_LOG_INFO("打开项目 {}", l_p);
          });
        }
    }
  };

  ImGui::Separator();

  if (dear::MenuItem("保存"s, "Ctrl+S")) g_reg()->ctx().at<core_sig>().save();

  ImGui::Separator();
  dear::MenuItem("调试"s, &p_i->p_debug_show);
  dear::MenuItem("样式设置"s, &p_i->p_style_show);
  dear::MenuItem("关于"s, &p_i->p_about_show);
  ImGui::Separator();
  if (dear::MenuItem(u8"退出")) {
    app_base::Get().stop_app();
  }
}

bool main_menu_bar::render() {
  if (ImGui::IsKeyPressed(ImGuiKey_S) && ImGui::GetIO().KeyCtrl) g_reg()->ctx().at<core_sig>().save();

  dear::Menu{"文件"} && [this]() { this->menu_file(); };
  dear::Menu{"窗口"} && [this]() { this->menu_windows(); };
  //  dear::Menu{"编辑"} && [this]() { this->menu_edit(); };
  dear::Menu{"工具"} && [this]() { this->menu_tool(); };
#ifndef NDEBUG
  ImGui::Text("%.3f ms/%.1f FPS", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
#endif

  return true;
}
void main_menu_bar::menu_edit() {}
void main_menu_bar::menu_windows() {
  dear::Menu{"布局"} && [this]() { this->menu_layout(); };
  for (auto &&[name, open] : g_windows_manage().get_menu_windows_list()) {
    if (dear::MenuItem(name.get().data(), open)) {
      if (*open)
        g_windows_manage().open_windows(name.get());
      else
        g_windows_manage().close_windows(name.get());
    }
    // g_windows_manage().show_windows(name.get());
  }
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
main_menu_bar::main_menu_bar(const main_menu_bar &in) noexcept : p_i(std::make_unique<impl>(*in.p_i)) {}
main_menu_bar::main_menu_bar(main_menu_bar &&in) noexcept { p_i = std::move(in.p_i); }
main_menu_bar &main_menu_bar::operator=(const main_menu_bar &in) noexcept {
  *p_i = *p_i;
  return *this;
}
main_menu_bar &main_menu_bar::operator=(main_menu_bar &&in) noexcept {
  p_i = std::move(in.p_i);
  return *this;
}

}  // namespace doodle::gui
