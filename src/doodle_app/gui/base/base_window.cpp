//
// Created by TD on 2022/4/8.
//

#include "base_window.h"

#include <doodle_core/core/core_set.h>
#include <doodle_core/core/init_register.h>

#include <doodle_app/app/app_command.h>
#include <doodle_app/gui/base/ref_base.h>

namespace doodle::gui {

windows_manage& g_windows_manage() { return doodle_lib::Get().ctx().get<windows_manage>(); }

class windows_manage::warp_w {
 public:
  windows_init_arg args_{};
  std::once_flag once_flag_size_{};
  std::once_flag once_flag_popup_{};
  windows win_render{};
  gui_cache_name_id title{};

  explicit warp_w(windows_init_arg in_arg) : args_(std::move(in_arg)) {}

  bool render() {
    //    std::call_once(once_flag_size_, [this]() { ImGui::SetNextWindowSize({args_.size_xy_[0], args_.size_xy_[1]});
    //    });

    switch (args_.render_enum_) {
      case windows_init_arg::render_enum::kpopup:
        ImGui::SetNextWindowSize({args_.size_xy_[0], args_.size_xy_[1]});
        break;
      case windows_init_arg::render_enum::kbegin:
        //        break;
      case windows_init_arg::render_enum::kmain_menu_bar:
        //      break;
      case windows_init_arg::render_enum::kviewport_side_bar:
        break;
    }

    auto l_win = args_.create_guard_(&args_);

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
      win_render = std::move((*args_.create_factory_)());
      win_render->render();
    }

    return args_.init_show_;
  };
};

void windows_manage::tick() {
  const render_guard l_g{this};
  windows_list_ |= ranges::actions::remove_if([](const warp_w_ptr& in_) { return !in_->render(); });
  windows_list_ |= ranges::actions::push_back(windows_list_next_);
  windows_list_next_.clear();
}

void windows_manage::create_windows_arg(const windows_init_arg& in_arg) {
  BOOST_ASSERT(!in_arg.title_.empty());
  BOOST_ASSERT(in_arg.create_factory_);
  BOOST_ASSERT(in_arg.create_guard_);
  auto l_arg = in_arg;
  switch (in_arg.render_enum_) {
    case windows_init_arg::render_enum::kpopup:
      l_arg.title_ = gui_cache_name_id{l_arg.title_}.name_id;
      break;
    case windows_init_arg::render_enum::kbegin:
      args_.emplace_back(l_arg);
      break;
    case windows_init_arg::render_enum::kmain_menu_bar:
      //      break;
    case windows_init_arg::render_enum::kviewport_side_bar:
      break;
  }
  if (l_arg.render_enum_ == windows_init_arg::render_enum::kbegin) args_.emplace_back(l_arg);
  if (!l_arg.init_show_) return;

  if (is_render_tick_p_)
    windows_list_next_.emplace_back(std::make_shared<warp_w>(l_arg));
  else
    windows_list_.emplace_back(std::make_shared<warp_w>(l_arg));
}

bool windows_manage::has_windows(const std::string_view& in_info) {
  return ranges::any_of(windows_list_next_, [&](const warp_w_ptr& i) { return i->args_.title_ == in_info; }) ||
         ranges::any_of(windows_list_, [&](const warp_w_ptr& i) { return i->args_.title_ == in_info; });
}

void windows_manage::show_windows(const std::string_view& in_info) {
  if (auto l_it = ranges::find_if(args_, [=](const windows_init_arg& in_arg) { return in_arg.title_ == in_info; });
      l_it != ranges::end(args_)) {
    auto l_arg       = *l_it;
    l_arg.init_show_ = true;
    if (is_render_tick_p_)
      windows_list_next_.emplace_back(std::make_shared<warp_w>(l_arg));
    else
      windows_list_.emplace_back(std::make_shared<warp_w>(l_arg));
  }
}

}  // namespace doodle::gui
