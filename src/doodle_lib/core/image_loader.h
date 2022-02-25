//
// Created by TD on 2022/1/21.
//

#pragma once
#include <doodle_lib/doodle_lib_fwd.h>
#include <opencv2/core.hpp>

namespace doodle {
class DOODLELIB_API image_loader {
  class impl;
  std::unique_ptr<impl> p_i;

  [[nodiscard]] std::shared_ptr<void> cv_to_d3d(const cv::Mat& in_mat, bool convert_toRGBA) const;

 public:
  class cache {
   public:
    std::shared_ptr<void> default_image;
    std::shared_ptr<void> error_image;
  };

  image_loader();
  virtual ~image_loader();

  /**
   * @brief 加载图片, 返回一个opencv对象和一个显卡资源句柄
   * @param in_path 路径. 相对于 project::p_path / "image" 的路径
   * @return cv 图片 和 显卡资源句柄
   */
  [[nodiscard("扔掉了加载的图片")]] std::tuple<cv::Mat, std::shared_ptr<void>> load_mat(const FSys::path& in_path);

  /**
   * @brief 加载图片
   * @param in_handle 具有 image_icon 组件的句柄
   * @return 是否加载成功
   */
  bool load(const entt::handle& in_handle);

  bool load(image_icon& in_icon);

  /**
   * @brief 保存图片
   * @param in_handle 将句柄中添加 image_icon
   * @return 是否保存成功
   */
  bool save(const entt::handle& in_handle, const cv::Mat& in_image, const cv::Rect2f& in_rect);
  /**
   * @brief 从路径中加载缩略图 并保存到库
   * @param in_handle  传入的句柄
   * @param in_path 传入的路径
   * @return 是否加载成功
   */
  bool save(const entt::handle& in_handle, const FSys::path& in);
  /**
   * @brief 将cv mat转换为本机指针
   * @param in_mat
   * @return
   */
  [[nodiscard]] std::shared_ptr<void> cv_to_d3d(const cv::Mat& in_mat) const;
  /**
   * @brief 从屏幕加载图片(截图)
   * @return opencv mat
   */
  [[nodiscard]] cv::Mat screenshot();
  [[nodiscard]] std::shared_ptr<void> default_image() const;
  [[nodiscard]] std::shared_ptr<void> error_image() const;
};
}  // namespace doodle
