//
// Created by TD on 2022/1/21.
//

#pragma once
#include <doodle_lib/doodle_lib_fwd.h>
namespace doodle {
class DOODLELIB_API image_loader {
  class impl;
  std::unique_ptr<impl> p_i;

 public:
  image_loader();
  virtual ~image_loader();

  /**
   * @brief 加载图片
   * @param in_handle 具有 image_icon 组件的句柄
   * @return 是否加载成功
   */
  bool load(const entt::handle& in_handle);
  /**
   * @brief 保存图片
   * @param in_handle 将句柄中添加 image_icon
   * @return 是否保存成功
   */
  bool save(const entt::handle& in_handle);
  /**
   * @brief 从屏幕加载图片(截图)
   * @return 本机指针
   */
  [[nodiscard]] std::shared_ptr<void> screenshot();
  [[nodiscard]] std::shared_ptr<void> default_image() const;
};
}  // namespace doodle
