//
// Created by TD on 2022/1/25.
//
#pragma once

#include <doodle_core/doodle_core_fwd.h>

#include <boost/signals2.hpp>

#include <entt/entt.hpp>

namespace doodle {
class project;

class DOODLE_CORE_API core_sig {
 public:
  /**
   * @brief 开始打开项目时发出的信号
   */
  boost::signals2::signal<void(const FSys::path&)> project_begin_open;
  /**
   * @brief 打开项目完成时发出的信号
   */
  boost::signals2::signal<void()> project_end_open;

  /**
   * @brief 当使用过滤器过滤时发出的信号
   *
   */
  boost::signals2::signal<void(const std::vector<entt::handle>&)> filter_handle;
  /**
   * @brief 最近选择的一个实体时发出的信号
   */
  boost::signals2::signal<void(const entt::handle&)> select_handle;
  /**
   * @brief 多个实体选中时发出的信号, 在单选时也会发出信号
   */
  boost::signals2::signal<void(const std::vector<entt::handle>&)> select_handles;

  /**
   * @brief 保存时发出的信号
   */
  boost::signals2::signal<void()> save;
};
}  // namespace doodle
