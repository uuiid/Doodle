//
// Created by TD on 2022/4/8.
//
#pragma once
#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_lib/lib_warp/imgui_warp.h>
#include <doodle_lib/gui/strand_gui.h>
#include <doodle_core/thread_pool/process_pool.h>
#include <doodle_core/core/init_register.h>
#include <doodle_core/gui_template/show_windows.h>
#include <boost/signals2.hpp>
#include <utility>

#include <boost/intrusive/list.hpp>
#include <boost/intrusive/list_hook.hpp>

#include <doodle_core/gui_template/gui_process.h>

namespace doodle::gui {
// namespace details {
// using hock_t = boost::intrusive::list_base_hook<
//     boost::intrusive::link_mode<
//         boost::intrusive::auto_unlink>>;
//
// }

namespace detail {

template <
    typename dear_type,
    typename windows_type>
class windows_tack_warp {
 protected:
  windows_type& self_() {
    return std::any_cast<windows_type&>(windows_self);
  };
  enum class state_enum : std::uint8_t {
    none = 0,
    set_attr,
    init
  };

  bool show_attr{};
  state_enum state_attr{};

  template <typename T = windows_type>
  auto call_self(std::integral_constant<state_enum, state_enum::set_attr>)
      -> decltype(std::declval<T>().init(), void()) {
    self_().set_attr();
  };

  template <typename T = windows_type>
  auto call_self(std::integral_constant<state_enum, state_enum::init>)
      -> decltype(std::declval<T>().init(), void()) {
    self_().init();
  };
  template <typename T = windows_type>
  auto call_self(...){};

 public:
  std::any windows_self;
  explicit windows_tack_warp(
      windows_type&& in_win
  ) : windows_self(std::move(in_win)){};
  virtual ~windows_tack_warp() = default;

  const std::string& title() {
    self_().title();
  }

  bool tick() {
    if (state_attr == state_enum::none) {
      call_self(std::integral_constant<
                state_enum, state_enum::set_attr>{});
      state_attr = state_enum::set_attr;
    }

    dear_type l_dear_{
        self_().title().data(),
        &show_attr,
        self_().flags()};

    if (l_dear_ && state_attr == state_enum::set_attr) {
      call_self(std::integral_constant<
                state_enum, state_enum::init>{});
      state_attr = state_enum::init;
    }

    l_dear_&& [this]() {
      self_().render();
    };
    if (!show_attr) {
      windows_self.reset();
    }

    return self_().show_;
  };
};

}  // namespace detail

using gui_tick    = doodle::gui::detail::windows_tick;
using gui_windows = doodle::gui::detail::windows_render;

}  // namespace doodle::gui
