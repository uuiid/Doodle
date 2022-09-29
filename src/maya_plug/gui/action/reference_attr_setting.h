//
// Created by TD on 2021/10/14.
//

#pragma once
#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_app/gui/base/base_window.h>
#include <maya_plug/configure/static_value.h>

namespace doodle::maya_plug {
class reference_file;

namespace reference_attr {
class data {
 public:
  std::string path;
  bool use_sim;
  bool operator==(const data& in_rhs) const;
  bool operator!=(const data& in_rhs) const;
  friend void to_json(nlohmann::json& j, const data& p) {
    j["path"]    = p.path;
    j["use_sim"] = p.use_sim;
  }
  friend void from_json(const nlohmann::json& j, data& p) {
    p.path    = j.at("path");
    p.use_sim = j.at("use_sim");
  }
};
using data_ptr = std::shared_ptr<data>;
}  // namespace reference_attr

/**
 * @brief 引用文件标签命令
 *
 * @li 添加引用文件解算标签
 * @li 在没有引用文件标记时， 使用所有载入的应用进行解算
 *
 */
class reference_attr_setting
    : public gui::base_windows<dear::Begin, reference_attr_setting> {
  class impl;
  std::unique_ptr<impl> p_i;

  bool get_file_info();
  void clear();

 public:
  reference_attr_setting();
  ~reference_attr_setting() override;
  constexpr static auto name = ::doodle::gui::config::maya_plug::menu::reference_attr_setting;

  void render()  ;
  const std::string& title() const override;
};

}  // namespace doodle::maya_plug
