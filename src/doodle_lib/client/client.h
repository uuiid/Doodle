//
// Created by TD on 2022/1/13.
//
#pragma once

#include <doodle_lib/doodle_lib_fwd.h>

namespace doodle::core {
class DOODLELIB_API client {
  void add_project(const FSys::path& in_path);

  class impl;
  std::unique_ptr<impl> p_i;
 public:
  /**
   * @brief 创建一个新的项目并打开
   * @param in_handle 传入的 句柄， 必须具有 project 组件
   */
  void new_project(const entt::handle& in_handle);
  void open_project(const FSys::path& in_path);
};
}  // namespace doodle::core
