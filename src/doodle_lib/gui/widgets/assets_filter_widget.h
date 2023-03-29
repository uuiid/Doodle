//
// Created by TD on 2021/9/16.
//

#pragma once
#include <doodle_core/core/init_register.h>

#include <doodle_app/gui/base/base_window.h>
#include <doodle_app/gui/base/ref_base.h>

#include <doodle_lib/doodle_lib_fwd.h>

#include <boost/signals2.hpp>

namespace doodle::gui {

/**
 * @brief 资产显示树
 * @li 这里只显示资产树, 可以类比为文件夹树
 *
 */
class DOODLELIB_API assets_filter_widget {
  class impl;
  std::unique_ptr<impl> p_impl;

  void refresh_(bool force);

 public:
  assets_filter_widget();
  ~assets_filter_widget();

  constexpr static std::string_view name{gui::config::menu_w::assets_filter};

  void init();
  bool render();
  const std::string& title() const;
  void refresh(bool force);
};

}  // namespace doodle::gui
