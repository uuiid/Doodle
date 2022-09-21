//
// Created by TD on 2022/4/8.
//
#pragma once
#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_lib/lib_warp/imgui_warp.h>
#include <doodle_lib/gui/strand_gui.h>
#include <doodle_core/thread_pool/process_pool.h>
#include <doodle_core/core/init_register.h>
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
    init
  };

  bool show_attr{};
  state_enum state_attr{};

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

  bool operator()() {
    dear_type l_dear_{
        self_().title().data(),
        &show_attr,
        self_().flags()};

    if (l_dear_ && state_attr == state_enum::none) {
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

/**
 * @brief 基本窗口
 */
class DOODLELIB_API base_window
    : public ::doodle::process_handy_tools {
 protected:
  std::vector<std::function<void()>> begin_fun{};
  std::vector<boost::signals2::scoped_connection> sig_scoped{};

  bool show_{true};

 public:
  using list        = std::set<base_window*>;
  using window_list = std::vector<std::shared_ptr<base_window>>;

  base_window();
  virtual ~base_window();

  /**
   * @brief 获取窗口标识
   * @return 窗口标识
   */
  [[nodiscard]] virtual const std::string& title() const = 0;

  boost::signals2::signal<void()> close{};
  /**
   * @brief 每帧渲染调用
   * @param in_duration 传入的时间间隔
   * @param in_data 传入的自定义数据
   */
  virtual void update() = 0;
  /**
   * @brief 判断是否显示
   * @return
   */
  [[nodiscard]] bool is_show() const;
  void show(bool in_show = true);
};

class DOODLELIB_API window_panel : public base_window {
 protected:
  std::string title_name_{};

  virtual void render() = 0;

 public:
  window_panel()           = default;
  ~window_panel() override = default;

  [[nodiscard]] const std::string& title() const override;
  void update() override;
};

class DOODLELIB_API modal_window : public base_window {
 protected:
  virtual void render() = 0;

 public:
  modal_window();
  ~modal_window() override = default;

  void update() override;
};

namespace base_windows_ns {
constexpr auto init_base_windows = []() {
  entt::meta<base_window>().type();
};

class init_base_windows_
    : public init_register::registrar_lambda<init_base_windows, 1> {};

constexpr auto init_windows_panel = []() {
  entt::meta<window_panel>().type().base<base_window>();
};

class init_windows_panel_
    : public init_register::registrar_lambda<init_windows_panel, 2> {};

}  // namespace base_windows_ns
}  // namespace doodle::gui
