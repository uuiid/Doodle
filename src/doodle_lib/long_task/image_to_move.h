//
// Created by TD on 2021/12/27.
//
#pragma once

#include <doodle_lib/doodle_lib_fwd.h>
#include <opencv2/core/types.hpp>
namespace doodle {
class DOODLELIB_API image_watermark {
 public:
  image_watermark() = default;
  image_watermark(std::string in_p_text, double_t in_p_width_proportion, double_t in_p_height_proportion, cv::Scalar in_rgba);
  std::string p_text;
  std::double_t p_width_proportion;
  std::double_t p_height_proportion;
  cv::Scalar rgba;
};

class DOODLELIB_API image_file_attribute {
  void extract_num_list();

 public:
  image_file_attribute();
  explicit image_file_attribute(FSys::path in_path);
  FSys::path file_path;
  std::vector<image_watermark> watermarks;
  std::vector<std::int32_t> num_list;
  std::int32_t num;

  static void extract_num(std::vector<image_file_attribute> &in_image_list);
  //  static bool is_image_list(const FSys::path &in_dir_path);
  bool operator<(const image_file_attribute &in_rhs) const;
  bool operator>(const image_file_attribute &in_rhs) const;
  bool operator<=(const image_file_attribute &in_rhs) const;
  bool operator>=(const image_file_attribute &in_rhs) const;
};
namespace details {

class DOODLELIB_API image_to_move : public process_t<image_to_move> {
  class impl;
  std::unique_ptr<impl> p_i;

 public:
  using base_type = process_t<image_to_move>;
  /**
   * @brief 将传入的图片序列连接为视频
   * @param in_handle 具有消息组件, 和 *输出路径文件夹* 组件的的句柄 可选的 shot， episode 组件
   * @param in_vector 这个就是之前传入图片属性
   *
   * @note 在传入的 in_handle 中， 我们会测试 shot， episode 组件， 如果具有这些组件，将会组合并进行重置输出路径的句柄
   */
  explicit image_to_move(const entt::handle &in_handle, const std::vector<image_file_attribute> &in_vector);
  /**
   * @brief 将传入的图片序列连接为视频(将具有默认的水印)
   * @param in_handle 具有消息组件, 和 *输出路径文件夹* 组件的的句柄 可选的 shot， episode 组件
   * @param in_vector 传入的图片路径
   *
   * @note 默认的水印是 @b 创建人的部门和姓名, 集数, 镜头号, 帧数/总帧数 ， 创建的时间
   */
  explicit image_to_move(const entt::handle &in_handle, const std::vector<FSys::path> &in_vector);

  static std::vector<image_file_attribute> make_default_attr(
      const entt::handle &in_handle,
      const std::vector<FSys::path> &in_path_list
  );

  inline static const cv::Scalar rgb_default{25, 220, 2};

  ~image_to_move() override;
  [[maybe_unused]] void init();
  [[maybe_unused]] void succeeded();
  [[maybe_unused]] void failed();
  [[maybe_unused]] void aborted();
  [[maybe_unused]] void update(const base_type::delta_type &, void *data);
};
}  // namespace details
using image_to_move = details::image_to_move;
}  // namespace doodle
