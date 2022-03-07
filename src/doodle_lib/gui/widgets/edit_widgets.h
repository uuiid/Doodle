//
// Created by TD on 2021/9/23.
//

#pragma once

#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_lib/gui/base_windwos.h>

#include <boost/signals2.hpp>
namespace doodle {
/**
 * @brief 各种编辑组件和创建句柄的窗口
 *
 */
class DOODLELIB_API edit_widgets : public process_t<edit_widgets> {
  class impl;
  std::unique_ptr<impl> p_i;

  void edit_handle();
  void add_handle();
  void clear_handle();
  void notify_file_list() const;

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
  ~edit_widgets() override;
  /**
   * @brief 窗口显示名称
   *
   */
  constexpr static std::string_view name{"编辑"};

  /**
   * @brief 初始化窗口
   *
   * 连接获取选择信号 获取 文件列表中的选择物体(单选)
   * 开始打开信号  在项目打开时清除获取的选中句柄, 并禁用编辑
   * 结束打开信号 启用编辑
   *
   *
   */
  [[maybe_unused]] void init();
  /**
   * @brief 成功结束任务
   *
   */
  [[maybe_unused]] void succeeded();
  /**
   * @brief 失败结束任务
   *
   */
  [[maybe_unused]] void failed();
  /**
   * @brief 用户中止时结束任务
   *
   */
  [[maybe_unused]] void aborted();
  /**
   * @brief 每帧刷新函数
   *
   * @param data 自定义数据
   */
  [[maybe_unused]] void update(const delta_type&, void* data);
};
}  // namespace doodle
