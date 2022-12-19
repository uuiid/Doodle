//
// Created by TD on 2022/4/8.
//
#pragma once
#include <doodle_core/core/init_register.h>
#include <doodle_core/gui_template/show_windows.h>

#include <doodle_app/doodle_app_fwd.h>
#include <doodle_app/lib_warp/imgui_warp.h>

#include <boost/signals2.hpp>

#include <utility>

namespace doodle::gui {
// namespace details {
// using hock_t = boost::intrusive::list_base_hook<
//     boost::intrusive::link_mode<
//         boost::intrusive::auto_unlink>>;
//
// }

namespace detail {

template <typename dear_type, typename windows_type>
class windows_tack_warp : public detail::windows_render_interface {
 protected:
  windows_type& self_() { return *dynamic_cast<windows_type*>(this); };
  enum class state_enum : std::uint8_t { none = 0, set_attr, init };

  bool show_attr{};
  state_enum state_attr{};

  template <typename T = windows_type>
  auto call_self(std::integral_constant<state_enum, state_enum::set_attr>)
      -> decltype(std::declval<T>().set_attr(), void()) {
    self_().set_attr();
  };

  template <typename T = windows_type>
  auto call_self(std::integral_constant<state_enum, state_enum::init>) -> decltype(std::declval<T>().init(), void()) {
    self_().init();
  };
  template <typename T = windows_type>
  auto call_self(...){};

  virtual std::int32_t flags() { return {}; };

 public:
  virtual ~windows_tack_warp() = default;

  bool tick() override {
    if (state_attr == state_enum::none) {
      call_self(std::integral_constant<state_enum, state_enum::set_attr>{});
      state_attr = state_enum::set_attr;
      show_attr  = true;
    }

    dear_type l_dear_{title().data(), &show_attr, self_().flags()};

    l_dear_&& [this]() {
      if (state_attr == state_enum::set_attr) {
        call_self(std::integral_constant<state_enum, state_enum::init>{});
        state_attr = state_enum::init;
      } else {
        self_().render();
      }
    };

    //    show_attr = ImGui::IsWindowAppearing();

    /// 显示时返回 false 不删除
    return !show_attr;
  };
};

std::tuple<entt::handle, doodle::gui::detail::windows_render> DOODLE_APP_API find_windows(const std::string& in_name);
}  // namespace detail
template <typename dear_type, typename windows_type>
using base_windows = doodle::gui::detail::windows_tack_warp<dear_type, windows_type>;
using gui_tick     = doodle::gui::detail::windows_tick;
using layout_tick  = doodle::gui::detail::layout_tick;
using gui_windows  = doodle::gui::detail::windows_render;

template <typename T, typename... Args>
std::tuple<entt::handle, std::shared_ptr<T>> show_windows(Args... in_arg) {
  if (auto&& [l_h, l_ptr] = doodle::gui::detail::find_windows(std::string{T::name}); l_h && l_ptr) {
    return std::make_tuple(l_h, std::dynamic_pointer_cast<T>(l_ptr));
  } else {
    auto l_h2 = make_handle();
    auto l_w  = l_h2.emplace<gui_windows>(std::make_shared<T>(std::forward<Args>(in_arg)...));
    return std::make_tuple(l_h2, std::dynamic_pointer_cast<T>(l_w));
  }
};

}  // namespace doodle::gui
