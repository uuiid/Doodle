//
// Created by TD on 2022/4/8.
//
#pragma once
#include <doodle_core/gui_template/show_windows.h>

#include <doodle_app/doodle_app_fwd.h>
#include <doodle_app/lib_warp/imgui_warp.h>

#include <boost/signals2.hpp>

#include <functional>
#include <memory>
#include <string_view>
#include <type_traits>
#include <utility>

namespace doodle::win {
class drop_manager;
}

namespace doodle::facet {
class gui_facet;
}
namespace doodle::gui {

namespace details {

#define DOODLE_HAS_VALUE(name)                                                        \
  template <typename type_t, typename = void>                                         \
  struct has_##name : std::false_type {};                                             \
  template <typename type_t>                                                          \
  struct has_##name<type_t, std::void_t<decltype(type_t::name)>> : std::true_type {}; \
  template <typename type_t>                                                          \
  static constexpr bool has_##name##_v = has_##name<type_t>::value;

DOODLE_HAS_VALUE(name);
DOODLE_HAS_VALUE(flags);
DOODLE_HAS_VALUE(sizexy);
DOODLE_HAS_VALUE(open_);

#undef DOODLE_HAS_VALUE
}  // namespace details

class windows_init_arg;
class layout_init_arg;

class windows_manage {
 public:
  class layout_info {
    friend windows_manage;

    std::shared_ptr<layout_init_arg> init_arg_;

   public:
    layout_info() = default;
    explicit layout_info(layout_init_arg& arg);
    std::string& name();
  };

 private:
  std::vector<windows_init_arg> args_{};

  class warp_w;
  using warp_w_ptr = std::shared_ptr<warp_w>;
  std::vector<warp_w_ptr> windows_list_{};
  std::vector<warp_w_ptr> windows_list_next_{};

  std::atomic_bool is_render_tick_p_{};
  gui::windows_layout layout_{};
  gui::windows_layout layout_next_{};
  facet::gui_facet* gui_facet_{};
  win::drop_manager* drop_manger_{};
  std::vector<std::function<void()>> close_fun_lists_{};
  std::vector<std::tuple<std::reference_wrapper<std::string>, bool*>> menu_list_{};
  std::vector<std::shared_ptr<layout_info>> layout_list_{};
  std::string render_layout_name_{};

  logger_ptr logger_;

  void gen_windows_list();
  class render_guard {
    windows_manage* ptr_;

   public:
    explicit render_guard(windows_manage* in_gui_facet) : ptr_(in_gui_facet) { ptr_->is_render_tick_p_ = true; }
    ~render_guard() { ptr_->is_render_tick_p_ = false; }
  };

  void set_layout(gui::windows_layout&& in_windows);

  void* get_windwos(const std::string_view& in_info);

  void close_all();

 public:
  explicit windows_manage(facet::gui_facet* in_facet);

  void create_windows_arg(const windows_init_arg& in_arg);

  void register_layout(gui::layout_init_arg& in_windows);
  void switch_layout(std::string_view in_name);

  void tick();
  bool has_windows(const std::string_view& in_info);
  void show_windows(const std::string_view& in_info);
  void close_windows(const std::string_view& in_info);
  void open_windows(const std::string_view& in_info);
  template <typename win_type>
  std::enable_if_t<details::has_name_v<win_type>, win_type*> find_windows() {
    if (has_windows(win_type::name)) {
      return static_cast<win_type*>(get_windwos(win_type::name));
    }
    return nullptr;
  };

  template <typename win_type>
  std::enable_if_t<details::has_name_v<win_type>> open_windows() {
    if (has_windows(win_type::name)) return;
    show_windows(win_type::name);
  };
  template <typename win_type>
  std::enable_if_t<details::has_name_v<win_type>> close_windows() {
    if (!has_windows(win_type::name)) return;
    close_windows(win_type::name);
  };

  std::vector<std::tuple<std::reference_wrapper<std::string>, bool*>>& get_menu_windows_list();
  std::vector<std::tuple<std::string_view, bool>> get_layout_list();

  void show_windows();

  /// \brief 用于在主线程中调用, 在窗口初始化时不可以向这个日志中打印
  [[nodiscard("")]] inline logger_ptr logger() const { return logger_; };
};

class windows_init_arg {
  friend class windows_manage;
  using dear_types = std::variant<dear::Popup, dear::PopupModal, dear::Begin, dear::MainMenuBar, dear::ViewportSideBar>;

  enum class render_enum : std::uint8_t {
    kpopup = 0,
    kbegin,
    kmain_menu_bar,
    kviewport_side_bar,
  };

 public:
  struct windows_t {
    std::shared_ptr<void> storage_;
    windows win_render_;
    inline bool render() { return win_render_->render(); }
    inline void reset() { storage_.reset(); }
    inline explicit operator bool() const { return static_cast<bool>(storage_); }
  };

 private:
  using set_open_ptr = void (*)(bool, std::shared_ptr<void>&);
  using create_ptr   = std::function<windows_t()>;
  using create_guard = std::function<dear_types(windows_init_arg*)>;
  /// 这个值会被显示, 更改布局, 用户主动显示等使用
  std::shared_ptr<bool> init_show_{std::make_shared<bool>(true)};
  render_enum render_enum_{};
  std::int32_t flags_{};
  std::array<float, 2> size_xy_{};
  std::string title_{};
  create_ptr create_factory_{};
  create_guard create_guard_{};
  set_open_ptr set_open{};

  template <typename type_t, std::enable_if_t<details::has_flags_v<type_t>>* = nullptr>
  inline windows_init_arg& get_default_flags() {
    flags_ = type_t::flags;
    return *this;
  }
  template <typename type_t, std::enable_if_t<!details::has_flags_v<type_t>>* = nullptr>
  inline windows_init_arg& get_default_flags() {
    return *this;
  }
  template <typename type_t, std::enable_if_t<details::has_sizexy_v<type_t>>* = nullptr>
  inline windows_init_arg& get_default_sizexy() {
    size_xy_ = type_t::sizexy;
    return *this;
  }
  template <typename type_t, std::enable_if_t<!details::has_sizexy_v<type_t>>* = nullptr>
  inline windows_init_arg& get_default_sizexy() {
    return *this;
  }

  template <typename type_t, std::enable_if_t<details::has_name_v<type_t>>* = nullptr>
  inline windows_init_arg& get_default_name() {
    title_ = std::string{type_t::name};
    return *this;
  }
  template <typename type_t, std::enable_if_t<!details::has_name_v<type_t>>* = nullptr>
  inline windows_init_arg& get_default_name() {
    return *this;
  }

  template <typename type_t, std::enable_if_t<details::has_open__v<type_t>>* = nullptr>
  inline windows_init_arg& set_default_open() {
    set_open = +[](bool in_open, std::shared_ptr<void>& in_win) {
      auto l_ptr   = std::static_pointer_cast<type_t>(in_win);
      l_ptr->open_ = in_open;
    };
    return *this;
  }
  template <typename type_t, std::enable_if_t<!details::has_open__v<type_t>>* = nullptr>
  inline windows_init_arg& set_default_open() {
    set_open = +[](bool in_open, std::shared_ptr<void>& in_win) {};
    return *this;
  }

 public:
  windows_init_arg() = default;

  template <typename render_type, std::enable_if_t<std::is_same_v<render_type, dear::Popup>>* = nullptr>
  inline windows_init_arg& set_render_type() {
    create_guard_ = [](windows_init_arg* in) {
      return dear_types{std::in_place_type_t<dear::Popup>{}, in->title_.data(), in->flags_};
    };
    render_enum_ = render_enum::kpopup;

    return *this;
  };

  template <typename render_type, std::enable_if_t<std::is_same_v<render_type, dear::PopupModal>>* = nullptr>
  inline windows_init_arg& set_render_type() {
    create_guard_ = [](windows_init_arg* in) {
      return dear_types{std::in_place_type_t<dear::PopupModal>{}, in->title_.data(), nullptr, in->flags_};
    };
    render_enum_ = render_enum::kpopup;

    return *this;
  };
  template <typename render_type, std::enable_if_t<std::is_same_v<render_type, dear::Begin>>* = nullptr>
  inline windows_init_arg& set_render_type() {
    create_guard_ = [](windows_init_arg* in) {
      return dear_types{std::in_place_type_t<dear::Begin>{}, in->title_.data(), in->init_show_.get(), in->flags_};
    };
    render_enum_ = render_enum::kbegin;
    return *this;
  };
  template <typename render_type, std::enable_if_t<std::is_same_v<render_type, dear::MainMenuBar>>* = nullptr>
  inline windows_init_arg& set_render_type() {
    create_guard_ = [](windows_init_arg*) { return dear_types{std::in_place_type_t<dear::MainMenuBar>{}}; };
    title_        = "dear::MainMenuBar";
    render_enum_  = render_enum::kmain_menu_bar;
    return *this;
  };
  template <typename render_type, std::enable_if_t<std::is_same_v<render_type, dear::ViewportSideBar>>* = nullptr>
  inline windows_init_arg& set_render_type(ImGuiViewport* viewport, const ImGuiDir& dir) {
    create_guard_ = [viewport, dir](windows_init_arg* in) {
      float height = ImGui::GetFrameHeight();
      return dear_types{std::in_place_type_t<dear::ViewportSideBar>{}, in->title_, viewport, dir, height, in->flags_};
    };
    render_enum_ = render_enum::kviewport_side_bar;
    return *this;
  };

  inline windows_init_arg& set_init_hide(bool is_show = false) {
    *init_show_ = is_show;
    return *this;
  };
  inline windows_init_arg& set_flags(std::int32_t in_flags) {
    flags_ = in_flags;
    return *this;
  };

  inline windows_init_arg& set_size(float in_x, float in_y) {
    size_xy_ = {in_x, in_y};
    return *this;
  }
  inline windows_init_arg& set_title(const std::string& in_string) {
    title_ = in_string;
    return *this;
  }

  template <typename t, typename... arg>
  std::enable_if_t<details::has_name_v<t>, windows_init_arg&> create_set_title(arg&&... in_arg) {
    create<t>(std::forward<arg&&>(in_arg)...);
    set_render_type<dear::Begin>();
    return *this;
  }

  template <typename t, typename... arg>
  windows_init_arg& create(arg&&... in_arg) {
    create_factory_ = [l_arg = std::make_tuple(std::move(in_arg)...)]() -> windows_t {
      return std::apply(
          [](auto&&... in_args) -> windows_t {
            auto l_win = std::make_shared<t>(std::move(in_args)...);
            return windows_t{l_win, windows{std::in_place_type<t&>, *l_win}};
          },
          std::move(l_arg)
      );
    };
    get_default_flags<t>();
    get_default_sizexy<t>();
    get_default_name<t>();
    set_default_open<t>();
    return *this;
  };
};
class layout_init_arg {
 private:
  friend windows_manage;
  std::string name_{};
  std::function<windows_layout()> layout_factory_{};

 public:
  layout_init_arg()                                  = default;
  layout_init_arg(const layout_init_arg&)            = default;
  layout_init_arg(layout_init_arg&&)                 = default;
  layout_init_arg& operator=(const layout_init_arg&) = default;
  layout_init_arg& operator=(layout_init_arg&&)      = default;

  template <typename layout_t>
  inline layout_init_arg& create(const std::string& in_name) {
    name_           = in_name;
    layout_factory_ = []() -> windows_layout { return windows_layout{std::in_place_type<layout_t>}; };
    return *this;
  }
  template <typename layout_t>
  inline std::enable_if_t<doodle::gui::details::has_name_v<layout_t>, layout_init_arg&> create() {
    return create<layout_t>(std::string{layout_t::name});
  }
};

windows_manage& g_windows_manage();

}  // namespace doodle::gui
