//
// Created by TD on 2021/10/14.
//

#pragma once

#include <doodle_app/gui/base/base_window.h>

#include <maya_plug/configure/static_value.h>

#include <maya/MObject.h>
namespace doodle::maya_plug {
class reference_file;

/**
 * @brief 引用文件标签命令
 *
 * @li 添加引用文件解算标签
 * @li 在没有引用文件标记时， 使用所有载入的应用进行解算
 *
 */
class reference_attr_setting {
  class impl;
  std::unique_ptr<impl> p_i;

  bool get_file_info();
  void add_collision(const MObject& in_obj);
  void get_collision(const MObject& in_obj);
  void add_wind_field(const MObject& in_obj);
  void set_attr(const MObject& in_obj, const std::string& in_attr_name, const std::string& in_value);

 public:
  reference_attr_setting();
  ~reference_attr_setting();
  constexpr static auto name = ::doodle::gui::config::maya_plug::menu::reference_attr_setting;

  bool render();
  const std::string& title() const;
};

}  // namespace doodle::maya_plug
