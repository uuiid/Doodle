//
// Created by TD on 2023/2/3.
//

#pragma once
#include <doodle_app/gui/base/base_window.h>
namespace doodle::work_task {
/**
 * @brief 设置窗口, 储存用户的 id, 名称, 部门, 以及属性等
 *
 * 临时作为登录的窗口, 后期可以进行分离
 */
class setting_windows {
  /// @todo 需要属性用户id(uuid)
  /// @todo 需要属性用户名称
  /// @todo 需要属性用户部门
  /// @todo 需要属性

 public:
  setting_windows();
  ~setting_windows();

  /// @todo 窗口id
  constexpr static std::string_view name{};
  const std::string& title() const;
  void init();
  /// @todo 这里需呀编辑
  bool render();
  /// @todo 保存函数
  void save();
};

}  // namespace doodle::work_task
