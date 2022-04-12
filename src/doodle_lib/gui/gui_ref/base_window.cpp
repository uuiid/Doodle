//
// Created by TD on 2022/4/8.
//

#include "base_window.h"
#include <doodle_lib/core/core_set.h>
#include <doodle_lib/core/init_register.h>
namespace doodle::gui {

void base_window::failed() {}
void base_window::update(const chrono::system_clock::duration &in_duration, void *in_data) {
  for (auto &&i : begin_fun) {
    i();
  }
  begin_fun.clear();

  dear::Begin{title().c_str(), &show} &&
      [&]() {
        this->render();
      };
}

base_window *base_window::find_window_by_title(const string &in_title) {
  auto &l_list = g_reg()->ctx_or_set<base_window::list>();
  auto it      = ranges::find_if(
           l_list,
           [&](const base_window *in_window) -> bool {
        return in_window->title() == in_title;
           });
  if (it != l_list.end())
    return *it;
  else
    return nullptr;
}
bool base_window::is_show() const {
  return show;
}

void window_panel::read_setting() {
  auto l_json = core_set::getSet().json_data;
  if (l_json->count(title()))
    (*l_json)[title()].get_to(setting);
  if (!setting.count("show"))
    setting["show"] = true;

  show = std::get<bool>(setting["show"]);
  if (!show)
    close();
  //  core_set_init{}.read_setting(title(), setting);
}
void window_panel::save_setting() const {
  auto l_json        = core_set::getSet().json_data;
  (*l_json)[title()] = setting;
  //  core_set_init{}.save_setting(title(), setting);
}
void window_panel::init() {
  read_setting();
}
void window_panel::succeeded() {
  save_setting();
}

void window_panel::aborted() {
  save_setting();
}
const string &window_panel::title() const {
  return title_name_;
}

modal_window::modal_window()
    : show{true} {
  begin_fun.emplace_back([this]() {
    ImGui::OpenPopup(title().data());
    ImGui::SetNextWindowSize({640, 360});
    close.connect([]() {
      ImGui::CloseCurrentPopup();
    });
  });
}
void modal_window::update(const chrono::system_clock::duration &in_dalta, void *in_data) {
  for (auto &&i : begin_fun) {
    i();
  }
  begin_fun.clear();

  dear::PopupModal{title().data(), &show} &&
      [&]() {
        render();
      };
}

void windows_proc::init() {
  chick_true<doodle_error>(owner_.owner(), DOODLE_LOC, "未获得窗口所有权");
  g_reg()->ctx_or_set<base_window::list>().emplace(windows_);
  this->warp_proc_->show  = true;
  this->warp_proc_->close = [this]() {
    this->windows_->close();
  };
  windows_->close.connect([this]() {
    this->succeed();
    this->warp_proc_->show = false;
  });
  windows_->init();
}
void windows_proc::succeeded() {
  windows_->succeeded();
  g_reg()->ctx_or_set<base_window::list>().erase(windows_);
}
void windows_proc::failed() {
  windows_->failed();
  g_reg()->ctx_or_set<base_window::list>().erase(windows_);
}
void windows_proc::aborted() {
  windows_->aborted();
  g_reg()->ctx_or_set<base_window::list>().erase(windows_);
}
void windows_proc::update(const chrono::system_clock::duration &in_duration,
                          void *in_data) {
  windows_->update(in_duration, in_data);
  if (!windows_->is_show())
    windows_->close();
}
windows_proc::~windows_proc() = default;
}  // namespace doodle::gui
