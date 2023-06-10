//
// Created by TD on 2021/9/23.
//

#pragma once

#include <doodle_app/gui/base/base_window.h>

#include <doodle_lib/doodle_lib_fwd.h>

#include <boost/signals2.hpp>
namespace doodle::gui {

namespace edit_widgets_ns {
class edit_assets_data {
 public:
  FSys::path old_path;
  std::string new_name;
};

}  // namespace edit_widgets_ns

/**
 * @brief 各种编辑组件和创建句柄的窗口
 *
 * 在没有计算出文件路径的时候, 其中,名称为空
 *
 */
class DOODLELIB_API edit_widgets {
  class impl;
  std::unique_ptr<impl> p_i;

  void edit_handle();

 public:
  /**
   * @brief Construct a new edit widgets object
   *
   */
  edit_widgets();
  /**
   * @brief Destroy the edit widgets object
   *
   */
  virtual ~edit_widgets();
  /**
   * @brief 窗口显示名称
   *
   */
  constexpr static std::string_view name{gui::config::menu_w::edit_};

  [[nodiscard("")]] const std::string& title() const;
  /**
   * @brief 每帧刷新函数
   *
   * @param data 自定义数据
   */
  bool render();
};

}  // namespace doodle::gui
