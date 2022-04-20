//
// Created by TD on 2022/4/13.
//

#include "layout_window.h"
namespace doodle::gui {
class layout_window::impl {
 public:
  impl() = default;
  std::map<std::string, std::unique_ptr<windows_proc>> list_windows{};

  chrono::system_clock::duration duration_{};
  void *data_{};

  std::function<void()> main_render{};
};
layout_window::layout_window()
    : p_i(std::make_unique<impl>()) {
}

const string &layout_window::title() const {
  static std::string l_title{"layout_window"};
  return l_title;
}

void layout_window::init() {
  auto k_list = init_register::instance().get_derived_class<gui::window_panel>();
  for (auto &&l_item : k_list) {
    if (auto l_win = l_item.construct(); l_win) {
      auto l_win_ptr = l_win.try_cast<base_window>();
      auto l_ptr     = std::make_shared<windows_proc::warp_proc>();
      this->p_i->list_windows.emplace(
          l_win_ptr->title(),
          std::make_unique<windows_proc>(
              l_ptr,
              l_win_ptr,
              std::move(l_win)));
    };
  }
  p_i->main_render = [this]() { call_render(std::string{gui::config::menu_w::assets_file}); };
  g_reg()->ctx().emplace<layout_window &>(*this);
}

void layout_window::succeeded() {
}

void layout_window::update(const chrono::system_clock::duration &in_duration,
                           void *in_data) {
  const ImGuiViewport *viewport = ImGui::GetMainViewport();
  ImGui::SetNextWindowPos(viewport->WorkPos);
  ImGui::SetNextWindowSize(viewport->WorkSize);
  p_i->duration_ = in_duration;
  p_i->data_     = in_data;
  dear::Begin{"main_windows", &show_,
              ImGuiWindowFlags_NoDecoration |
                  ImGuiWindowFlags_NoMove |
                  ImGuiWindowFlags_NoResize |
                  ImGuiWindowFlags_NoSavedSettings} &&
      [&, this]() {
        namespace menu_w = gui::config::menu_w;
        dear::Child{"l1", ImVec2{viewport->WorkSize.x / 4, 0}, false} && [&, this]() {
          dear::Child{"ll1", ImVec2{0, viewport->WorkSize.y / 6}} && [&]() { call_render(std::string{menu_w::project_widget}); };
          dear::Child{"ll2", ImVec2{0, viewport->WorkSize.y / 3}} && [&]() { call_render(std::string{menu_w::assets_filter}); };
          dear::Child{"ll3"} && [&]() { call_render(std::string{menu_w::edit_}); };
        };
        ImGui::SameLine();
        dear::Child{"l2", ImVec2{viewport->WorkSize.x / 3, 0}, true} && [&, this]() {
          p_i->main_render();
        };
        ImGui::SameLine();
        dear::Child{"l3", ImVec2{0, 0}, true} && [&, this]() {
          dear::TabBar{"##tool",
                       ImGuiTabBarFlags_Reorderable |
                           ImGuiTabBarFlags_FittingPolicyResizeDown} &&
              [&, this]() {
                dear::TabItem{menu_w::csv_export.data()} && [&]() { call_render(std::string{menu_w::csv_export}); };
                dear::TabItem{menu_w::ue4_widget.data()} && [&]() { call_render(std::string{menu_w::ue4_widget}); };
                dear::TabItem{menu_w::comm_maya_tool.data()} && [&]() { call_render(std::string{menu_w::comm_maya_tool}); };
                dear::TabItem{menu_w::comm_create_video.data()} && [&]() { call_render(std::string{menu_w::comm_create_video}); };
                dear::TabItem{menu_w::extract_subtitles.data()} && [&]() { call_render(std::string{menu_w::extract_subtitles}); };
              };
        };
      };
  clear_windows();
}

void layout_window::call_render(const string &in_name) {
  auto &&l_win = p_i->list_windows[in_name];
  if (l_win)
    l_win->tick(p_i->duration_,
                p_i->data_);
}

std::shared_ptr<windows_proc::warp_proc>
layout_window::render_main(const string &in_name) {
  if (auto &&l_i = p_i->list_windows[in_name]; l_i) {
    l_i->windows_->close();
    call_render(in_name);
    clear_windows();
  }

  auto l_show = std::make_shared<windows_proc::warp_proc>();
  auto k_list = init_register::instance().get_derived_class<gui::window_panel>();

  auto l_it   = ranges::find_if(k_list, [&](const entt::meta_type &in_item) -> bool {
    return in_item.prop("name"_hs).value() == in_name;
  });
  if (l_it != k_list.end()) {
    if (auto l_win = l_it->construct(); l_win) {
      p_i->list_windows[in_name] = std::make_unique<windows_proc>(
          l_show,
          l_win.try_cast<base_window>(),
          std::move(l_win));
    }
  }

  if (auto &&l_i = p_i->list_windows[in_name]; l_i) {
    l_i->windows_->close.connect([this]() {
      p_i->main_render = [this]() { call_render(std::string{gui::config::menu_w::assets_file}); };
    });
    l_show = l_i->warp_proc_;
  }

  p_i->main_render = [this, l_show, in_name]() {
    call_render(in_name);
  };
  return l_show;
}
void layout_window::clear_windows() {
  for (auto it = p_i->list_windows.begin(); it != p_i->list_windows.end();) {
    if (!it->second || it->second->finished() || it->second->rejected()) {
      it = p_i->list_windows.erase(it);
    } else {
      ++it;
    }
  }
}
layout_window::~layout_window() = default;
}  // namespace doodle::gui
