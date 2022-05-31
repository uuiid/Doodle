//
// Created by TD on 2022/1/14.
//

#include "main_menu_bar.h"
#include <lib_warp/imgui_warp.h>
#include <doodle_lib/app/app.h>

#include <doodle_core/client/client.h>
#include <gui/open_file_dialog.h>
#include <toolkit/toolkit.h>
#include <gui/setting_windows.h>
#include <gui/get_input_dialog.h>
#include <gui/gui_ref/ref_base.h>
#include <doodle_core/core/core_sig.h>
#include <doodle_lib/gui/gui_ref/layout_window.h>

namespace doodle {
namespace main_menu_bar_ns {
void to_json(nlohmann::json &j, const layout_data &p) {
  j["imgui_data"]   = p.imgui_data;
  j["windows_show"] = p.windows_show;
  j["name"]         = p.name;
}
void from_json(const nlohmann::json &j, layout_data &p) {
  j["imgui_data"].get_to(p.imgui_data);
  j["windows_show"].get_to(p.windows_show);
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
  std::map<std::string, std::shared_ptr<gui::windows_proc::warp_proc>> windows_;

  gui::gui_cache<std::string> layout_name_{"##name"s, "layout_name"s};
  gui::gui_cache_name_id button_save_layout_name_{"保存"};
  gui::gui_cache_name_id button_delete_layout_name_{"删除当前布局"};

  std::vector<main_menu_bar_ns::layout_data> layout_list;
};

void to_json(nlohmann::json &j, const main_menu_bar &p) {
  j["layout_list"] = p.p_i->layout_list;
}
void from_json(const nlohmann::json &j, main_menu_bar &p) {
  if (j.count("layout_list"))
    j["layout_list"].get_to(p.p_i->layout_list);
}

main_menu_bar::main_menu_bar()
    : p_i(std::make_unique<impl>()) {
  this->show_ = true;
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
  std::apply([this](const auto &...in_item) {
    (this->widget_menu_item(in_item), ...);
  },
             std::make_tuple(gui::config::menu_w::setting, gui::config::menu_w::project_edit));
}
void main_menu_bar::widget_menu_item(const std::string_view &in_view) {
  std::string key{in_view};
  auto &&l_win      = this->p_i->windows_[key];
  const auto l_show = l_win ? l_win->is_show() : false;
  if (dear::MenuItem(in_view.data(), l_show)) {
    if (l_show) {
      l_win->close();
    } else {
      this->p_i->windows_[key] = g_reg()->ctx().at<gui::layout_window>().render_main(key);
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
  this->read_setting();
  g_reg()->ctx().emplace<main_menu_bar &>(*this);
}
void main_menu_bar::update(
    const chrono::system_clock::duration &in_duration,
    void *in_data) {
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
void main_menu_bar::succeeded() {
  show_ = true;
  save_setting();
}
const std::string &main_menu_bar::title() const {
  static std::string name{"main_menu"};
  return name;
}
void main_menu_bar::aborted() {
  base_window::aborted();
  save_setting();
}
void main_menu_bar::read_setting() {
  base_window::read_setting();
  if (auto &&l_j = get_setting(); l_j.count("main_menu_bar"))
    l_j["main_menu_bar"].get_to(*this);
}
void main_menu_bar::save_setting() const {
  base_window::save_setting();
  get_setting()["main_menu_bar"] = *this;
}

}  // namespace doodle
