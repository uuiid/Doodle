//
// Created by TD on 2021/9/16.
//

#pragma once
#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_lib/gui/gui_ref/base_window.h>
#include <doodle_lib/gui/gui_ref/ref_base.h>
#include <doodle_core/core/init_register.h>

#include <boost/signals2.hpp>

namespace doodle {

/**
 * @brief 资产显示树
 * @li 这里只显示资产树, 可以类比为文件夹树
 *
 */
class DOODLELIB_API assets_filter_widget
    : public gui::base_windows<dear::Begin, assets_filter_widget> {
  class impl;
  std::unique_ptr<impl> p_impl;

  void refresh_(bool force);

 public:
  assets_filter_widget();
  ~assets_filter_widget() override;

  constexpr static std::string_view name{gui::config::menu_w::assets_filter};

  void init();
  void render();
  const std::string& title() const override;
  void refresh(bool force);
};

}  // namespace doodle
