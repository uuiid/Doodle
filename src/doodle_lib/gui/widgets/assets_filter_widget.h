//
// Created by TD on 2021/9/16.
//

#pragma once
#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_lib/gui/gui_ref/base_window.h>
#include <doodle_lib/gui/gui_ref/ref_base.h>
#include <doodle_lib/core/init_register.h>

#include <boost/signals2.hpp>


namespace doodle {

namespace gui {




}  // namespace gui

/**
 * @brief 资产显示树
 * @li 这里只显示资产树, 可以类比为文件夹树
 *
 */
class DOODLELIB_API assets_filter_widget
    : public gui::window_panel {
  class impl;
  std::unique_ptr<impl> p_impl;

  void refresh_(bool force);

 public:
  assets_filter_widget();
  ~assets_filter_widget() override;

  constexpr static std::string_view name{gui::config::menu_w::assets_filter};

  void init() override;
  void failed() override;
  void render() override;

  void refresh(bool force);
};

namespace assets_filter_widget_ns {
constexpr auto init = []() {
  entt::meta<assets_filter_widget>()
      .type()
      .prop("name"_hs, std::string{assets_filter_widget::name})
      .base<gui::window_panel>();
};
class init_class
    : public init_register::registrar_lambda<init, 3> {};
}  // namespace assets_filter_widget_ns

}  // namespace doodle
