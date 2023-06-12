//
// Created by TD on 2022/4/8.
//

#include "base_window.h"

#include <doodle_core/core/core_set.h>
#include <doodle_core/core/init_register.h>

#include <doodle_app/app/app_command.h>
#include <doodle_app/gui/base/ref_base.h>

#include "imgui.h"
#include "platform/win/drop_manager.h"
#include "range/v3/action/remove_if.hpp"
#include "range/v3/action/sort.hpp"
#include "range/v3/algorithm/none_of.hpp"
#include <any>
#include <memory>
#include <string_view>
#include <utility>

namespace doodle::gui {

windows_manage& g_windows_manage() { return doodle_lib::Get().ctx().get<windows_manage>(); }

windows_manage::layout_info::layout_info(doodle::gui::layout_init_arg& arg)
    : init_arg_(std::make_shared<layout_init_arg>(std::move(arg))) {}

std::string& windows_manage::layout_info::name() { return init_arg_->name_; }

class windows_manage::warp_w {
 public:
  windows_init_arg args_{};
  std::once_flag once_flag_size_{};
  std::once_flag once_flag_popup_{};
  windows win_render{};
  gui_cache_name_id title{};

  explicit warp_w(windows_init_arg in_arg) : args_(std::move(in_arg)) {}
  ~warp_w() { *args_.init_show_ = false; }

  bool render() {
    switch (args_.render_enum_) {
      case windows_init_arg::render_enum::kpopup:
        ImGui::SetNextWindowSize({args_.size_xy_[0], args_.size_xy_[1]}, ImGuiCond_Once);
        //        ImGui::SetNextWindowSize({args_.size_xy_[0], args_.size_xy_[1]});
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
      return *args_.init_show_;
    }

    if (win_render) {
      l_show = win_render->render();
    } else {
      win_render = std::move((*args_.create_factory_)());
      win_render->render();
    }

    return *args_.init_show_ && l_show;
  };
};

void windows_manage::tick() {
  if (layout_next_) {
    std::swap(layout_next_, layout_);
    layout_next_.reset();
    layout_->set_show();
  }
  layout_->render();

  drop_manger_->render();

  const render_guard l_g{this};
  const auto l_org_list = windows_list_.size();
  windows_list_ |= ranges::actions::remove_if([](const warp_w_ptr& in_) { return !in_->render(); });

  windows_list_ |= ranges::actions::push_back(windows_list_next_);
  windows_list_next_.clear();
}

windows_manage::windows_manage(facet::gui_facet* in_facet)
    : gui_facet_(in_facet), drop_manger_(in_facet ? in_facet->drop_manager() : nullptr) {}

void windows_manage::show_windows() {
  if (gui_facet_)
    gui_facet_->show_windows();
  else
    DOODLE_LOG_INFO("gui 构面为空, 不打开主窗口");
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
      gen_windows_list();
      break;
    case windows_init_arg::render_enum::kmain_menu_bar:
      //      break;
    case windows_init_arg::render_enum::kviewport_side_bar:
      break;
  }

  if (!*l_arg.init_show_) return;

  if (is_render_tick_p_) {
    windows_list_next_.emplace_back(std::make_shared<warp_w>(l_arg));
  } else {
    windows_list_.emplace_back(std::make_shared<warp_w>(l_arg));
  }
}

bool windows_manage::has_windows(const std::string_view& in_info) {
  return ranges::any_of(windows_list_next_, [&](const warp_w_ptr& i) { return i->args_.title_ == in_info; }) ||
         ranges::any_of(windows_list_, [&](const warp_w_ptr& i) { return i->args_.title_ == in_info; });
}

void windows_manage::set_layout(gui::windows_layout&& in_windows) {
  BOOST_ASSERT(in_windows);
  if (!layout_) {
    layout_ = std::move(in_windows);
    layout_->set_show();
  } else {
    layout_next_ = std::move(in_windows);
  }
}

void windows_manage::register_layout(gui::layout_init_arg& in_windows) {
  BOOST_ASSERT(in_windows.layout_factory_);
  BOOST_ASSERT(ranges::none_of(layout_list_, [&](const decltype(layout_list_)::value_type& in) -> bool {
    return in->init_arg_->name_ == in_windows.name_;
  }));

  layout_list_.emplace_back(std::make_shared<layout_info>(in_windows));
}
void windows_manage::switch_layout(std::string_view in_name) {
  if (auto l_it = ranges::find_if(
          layout_list_,
          [&](const decltype(layout_list_)::value_type& in) -> bool { return in->init_arg_->name_ == in_name; }
      );
      l_it != layout_list_.end()) {
    render_layout_name_ = in_name;
    set_layout((*l_it)->init_arg_->layout_factory_());
  }
}

void windows_manage::show_windows(const std::string_view& in_info) {
  if (auto l_it = ranges::find_if(args_, [=](const windows_init_arg& in_arg) { return in_arg.title_ == in_info; });
      l_it != ranges::end(args_)) {
    auto l_arg        = *l_it;
    *l_arg.init_show_ = true;
    if (is_render_tick_p_) {
      windows_list_next_.emplace_back(std::make_shared<warp_w>(l_arg));
    } else {
      windows_list_.emplace_back(std::make_shared<warp_w>(l_arg));
    }
  }
}

void windows_manage::close_windows(const std::string_view& in_info) {
  if (auto l_it =
          ranges::find_if(windows_list_, [=](const warp_w_ptr& in_arg) { return in_arg->args_.title_ == in_info; });
      l_it != ranges::end(windows_list_)) {
    auto l_arg               = *l_it;
    *l_arg->args_.init_show_ = false;
  }
  windows_list_next_ |=
      ranges::actions::remove_if([=](const warp_w_ptr& in_arg) { return in_arg->args_.title_ == in_info; });
}
void windows_manage::open_windows(const std::string_view& in_info) {
  if (has_windows(in_info)) return;
  show_windows(in_info);
}

std::vector<std::tuple<std::reference_wrapper<std::string>, bool*>>& windows_manage::get_menu_windows_list() {
  return menu_list_;
}

std::vector<std::tuple<std::string_view, bool>> windows_manage::get_layout_list() {
  return layout_list_ | ranges::views::transform([this](const decltype(layout_list_)::value_type& i) {
           return std::make_tuple(std::string_view{i->init_arg_->name_}, render_layout_name_ == i->init_arg_->name_);
         }) |
         ranges::to_vector;
}

void windows_manage::gen_windows_list() {
  menu_list_.clear();
  for (auto&& i : args_) {
    if (i.render_enum_ == windows_init_arg::render_enum::kbegin)
      menu_list_.emplace_back(std::ref(i.title_), i.init_show_.get());
  }
  menu_list_ |= ranges::actions::sort([](const decltype(menu_list_)::value_type& in_r,
                                         const decltype(menu_list_)::value_type& in_l) {
    return std::get<0>(in_r).get() < std::get<0>(in_l).get();
  });
}
}  // namespace doodle::gui
