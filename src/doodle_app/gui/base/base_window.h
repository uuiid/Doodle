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

namespace doodle::facet {
class gui_facet;
}
namespace doodle::gui {

namespace details {
template <typename type_t, typename = void>
struct has_name : std::false_type {};
template <typename type_t>
struct has_name<type_t, std::void_t<decltype(type_t::name)>> : std::true_type {};
template <typename type_t>
static constexpr bool has_name_v = has_name<type_t>::value;
}  // namespace details

class windows_init_arg;

class windows_manage {
  std::vector<windows_init_arg> args_{};

  class warp_w;
  using warp_w_ptr = std::shared_ptr<warp_w>;
  std::vector<warp_w_ptr> windows_list_{};
  std::vector<warp_w_ptr> windows_list_next_{};

  std::atomic_bool is_render_tick_p_;

  class render_guard {
    windows_manage* ptr_;

   public:
    explicit render_guard(windows_manage* in_gui_facet) : ptr_(in_gui_facet) { ptr_->is_render_tick_p_ = true; }
    ~render_guard() { ptr_->is_render_tick_p_ = false; }
  };

  bool has_windows(const std::string_view& in_info);
  void show_windows(const std::string_view& in_info);

 public:
  windows_manage() = default;

  void create_windows_arg(const windows_init_arg& in_arg);

  void tick();
  template <typename T>
  std::enable_if_t<details::has_name_v<T>> open_windows() {
    if (has_windows(T::name)) return;
    show_windows(T::name);
  };
};

class windows_init_arg {
  friend class windows_manage;

  std::array<float, 2> size_xy_{};
  std::string title_{};
  bool init_show_{true};

  std::shared_ptr<std::function<windows()>> create_factory_{};

  using dear_types = std::variant<dear::Popup, dear::Begin, dear::MainMenuBar, dear::ViewportSideBar>;
  std::function<dear_types(windows_init_arg*)> create_guard;

  std::int32_t flags_{};

 public:
  windows_init_arg() = default;

  template <typename render_type, std::enable_if_t<std::is_same_v<render_type, dear::Popup>>* = nullptr>
  inline windows_init_arg& set_render_type() {
    create_guard = [](windows_init_arg* in) {
      return dear_types{std::in_place_type_t<dear::Popup>{}, in->title_.data(), in->flags_};
    };
    return *this;
  };
  template <typename render_type, std::enable_if_t<std::is_same_v<render_type, dear::Begin>>* = nullptr>
  inline windows_init_arg& set_render_type() {
    create_guard = [](windows_init_arg* in) {
      return dear_types{std::in_place_type_t<dear::Begin>{}, in->title_.data(), &in->init_show_, in->flags_};
    };
    return *this;
  };
  template <typename render_type, std::enable_if_t<std::is_same_v<render_type, dear::MainMenuBar>>* = nullptr>
  inline windows_init_arg& set_render_type() {
    create_guard = [](windows_init_arg*) { return dear_types{std::in_place_type_t<dear::MainMenuBar>{}}; };
    title_       = "dear::MainMenuBar";
    return *this;
  };
  template <typename render_type, std::enable_if_t<std::is_same_v<render_type, dear::ViewportSideBar>>* = nullptr>
  inline windows_init_arg& set_render_type(ImGuiViewport* viewport, const ImGuiDir& dir) {
    create_guard = [viewport, dir](windows_init_arg* in) {
      float height = ImGui::GetFrameHeight();
      return dear_types{std::in_place_type_t<dear::ViewportSideBar>{}, in->title_, viewport, dir, height, in->flags_};
    };
    return *this;
  };

  inline windows_init_arg& set_init_show(bool is_show = true) {
    init_show_ = is_show;
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
  template <typename win_type, std::enable_if_t<details::has_name_v<win_type>>* = nullptr>
  inline windows_init_arg& set_title() {
    title_ = std::string{win_type::name};
    return *this;
  }

  template <typename t, typename... arg>
  std::enable_if_t<details::has_name_v<t>, windows_init_arg&> create_set_title(arg&&... in_arg) {
    create<t>(std::forward<arg&&>(in_arg)...);
    set_title<t>();
    set_render_type<dear::Begin>();
    return *this;
  }

  template <typename t, typename... arg>
  windows_init_arg& create(arg&&... in_arg) {
    create_factory_ = std::make_shared<decltype(create_factory_)::element_type>(
        [l_arg = std::make_tuple(std::move(in_arg)...)]() -> windows {
          return std::apply(
              [](auto&&... in_args) -> windows {
                return windows{std::in_place_type<t>, std::move(in_args)...};
              },
              std::move(l_arg)
          );
        }
    );
    return *this;
  };
};

windows_manage& g_windows_manage();

}  // namespace doodle::gui
