//
// Created by TD on 2022/4/8.
//

#include "base_window.h"

#include <doodle_core/core/core_set.h>
#include <doodle_core/core/init_register.h>

#include <doodle_app/app/app_command.h>
#include <doodle_app/app/facet/gui_facet.h>

namespace doodle::gui {

windows_manage& g_windows_manage() { return doodle_lib::Get().ctx().get<windows_manage>(); }

class windows_manage::warp_w {
 public:
  windows_init_arg args_{};
  std::once_flag once_flag_size_{};
  std::once_flag once_flag_popup_{};
  windows win_render{};
  explicit warp_w(windows_init_arg&& in_arg) : args_(std::move(in_arg)) {}

  bool render() {
    auto l_win = args_.create_guard();

    std::call_once(once_flag_size_, [this]() { ImGui::SetNextWindowSize({args_.size_xy_[0], args_.size_xy_[1]}); });
    bool l_show{};
    std::visit(
        entt::overloaded{
            [l_s = &l_show, this](dear::Popup& in) {
              *l_s = in;
              std::call_once(once_flag_popup_, [this]() { ImGui::OpenPopup(args_.title_.data()); });
            },
            [l_s = &l_show](dear::Begin& in) { *l_s = in; },
            [l_s = &l_show](dear::MainMenuBar& in) { *l_s = in; },
            [l_s = &l_show](dear::ViewportSideBar& in) { *l_s = in; },
        },
        l_win
    );
    if (!l_show) {
      win_render.reset();
      return args_.init_show_;
    }

    if (win_render) {
      win_render->render();
    } else {
      win_render = std::move(args_.create_factory_());
      win_render->render();
    }

    return args_.init_show_;
  };
};

void windows_manage::create_windows_arg(windows_init_arg& in_arg) {
  if (gui_facet.get().is_render_tick())
    gui_facet.get().windows_list_next.emplace_back(std::in_place_type<warp_w>, std::move(in_arg));
  else
    gui_facet.get().windows_list.emplace_back(std::in_place_type<warp_w>, std::move(in_arg));
}

bool windows_manage::has_windows(const entt::type_info& in_info) {
  return ranges::any_of(gui_facet.get().windows_list_next, [&](windows& i) { return i.type() == in_info; }) ||
         ranges::any_of(gui_facet.get().windows_list, [&](windows& i) { return i.type() == in_info; });
}

void windows_manage::create_windows_(windows&& in_windows) {
  if (gui_facet.get().is_render_tick())
    gui_facet.get().windows_list_next.emplace_back(std::move(in_windows));
  else
    gui_facet.get().windows_list.emplace_back(std::move(in_windows));
}

}  // namespace doodle::gui
