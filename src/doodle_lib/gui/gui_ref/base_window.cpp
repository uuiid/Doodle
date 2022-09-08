//
// Created by TD on 2022/4/8.
//

#include "base_window.h"
#include <doodle_core/core/core_set.h>
#include <doodle_core/core/init_register.h>
namespace doodle::gui {

void to_json(nlohmann::json &j, const base_window &p) {
}
void from_json(const nlohmann::json &j, base_window &p) {
}

base_window *base_window::find_window_by_title(const std::string &in_title) {
  auto &l_list = g_reg()->ctx().emplace<base_window::list>();
  auto it      = ranges::find_if(
      l_list,
      [&](const base_window *in_window) -> bool {
        return in_window->title() == in_title;
      }
  );
  if (it != l_list.end())
    return *it;
  else
    return nullptr;
}
bool base_window::is_show() const {
  return show_;
}
void base_window::show(bool in_show) {
  show_ = in_show;
}

nlohmann::json &base_window::get_setting() const {
  return (*core_set::get_set().json_data)[title()];
}
base_window::base_window()  = default;
base_window::~base_window() = default;

const std::string &window_panel::title() const {
  return title_name_;
}
void window_panel::update() {
  for (auto &&i : begin_fun) {
    i();
  }
  begin_fun.clear();

  dear::Begin{title().c_str(), &show_} &&
      [&]() {
        this->render();
      };

  if (!show_) {
    succeed();
  }
}

modal_window::modal_window() {
  begin_fun.emplace_back([this]() {
    ImGui::OpenPopup(title().data());
    ImGui::SetNextWindowSize({640, 360});
  });
  close.connect([this]() { ImGui::CloseCurrentPopup(); });
}
void modal_window::update() {
  for (auto &&i : begin_fun) {
    i();
  }
  begin_fun.clear();

  dear::PopupModal{title().data(), &show_, ImGuiWindowFlags_NoSavedSettings} &&
      [&]() {
        render();
      };
  if (!show_) {
    succeed();
  }
}

}  // namespace doodle::gui
