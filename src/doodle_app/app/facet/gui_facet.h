//
// Created by TD on 2022/9/30.
//

#pragma once
#include <doodle_core/core/app_facet.h>
#include <doodle_core/doodle_core.h>
#include <doodle_core/gui_template/show_windows.h>
#include <doodle_core/platform/win/windows_alias.h>

#include <doodle_app/doodle_app_fwd.h>

#include <entt/entt.hpp>
#include <tuple>

namespace doodle::gui {
class windows_manage;
}

namespace doodle::facet {

namespace details {
template <typename... Type>
[[maybe_unused]] entt::type_list<Type...> as_type_list(const entt::type_list<Type...>&);
class gui_facet_interface_1 : public entt::type_list_cat<
                                  decltype(as_type_list(std::declval<doodle::details::app_facet_interface_1>())),
                                  entt::type_list<void(gui::windows&& in_gui)>> {
 public:
  template <typename Base>
  struct type : doodle::details::app_facet_interface_1::template type<Base> {
    static constexpr auto base = decltype(as_type_list(std::declval<doodle::details::app_facet_interface_1>()))::size;
    //    static constexpr auto base =
    //        std::tuple_size_v<typename entt::poly_vtable<typename doodle::details::app_facet_interface_1, 12,
    //        32>::type>;
    void add_windows(gui::windows&& in_gui) { entt::poly_call<base>(*this, std::move(in_gui)); }
  };

  template <typename Type>
  using impl = entt::value_list_cat_t<
      typename doodle::details::app_facet_interface_1::impl<Type>, entt::value_list<&Type::add_windows>>;
};

}  // namespace details

class DOODLE_APP_API gui_facet {
  class impl;
  std::unique_ptr<impl> p_i;
  gui::windows layout_;
  std::atomic_bool is_render_tick_p;

  class render_guard {
    gui_facet* gui_facet_ptr;

   public:
    explicit render_guard(gui_facet* in_gui_facet) : gui_facet_ptr(in_gui_facet) {
      gui_facet_ptr->is_render_tick_p = true;
    }
    ~render_guard() { gui_facet_ptr->is_render_tick_p = false; }
  };

  friend class doodle::gui::windows_manage;

 protected:
  virtual bool translate_message();
  virtual void tick();
  virtual void tick_end();
  void drop_files();
  void external_update_mouse_coordinates(DWORD grfKeyState, POINTL in_point);

  ::doodle::win::wnd_handle p_hwnd;
  ::doodle::win::wnd_class p_win_class;
  void init_windows();

  virtual void load_windows() = 0;

  std::vector<gui::windows> windows_list{};

 public:
  gui_facet();
  virtual ~gui_facet();

  virtual void show_windows() const;
  virtual void close_windows();
  virtual void destroy_windows();

  inline bool is_render_tick() const { return is_render_tick_p; };

  inline void set_layout(gui::windows&& in_windows) { layout_ = std::move(in_windows); };

  std::vector<gui::windows> windows_list_next{};
  void set_title(const std::string& in_title) const;

  [[nodiscard]] const std::string& name() const noexcept;
  bool post();
  void deconstruction();
  virtual void add_program_options(){};
};
}  // namespace doodle::facet
