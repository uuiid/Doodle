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

class DOODLELIB_API image_to_move {
  class impl;
  std::unique_ptr<impl> p_i;

  void create_move(
      const entt::handle &in_handle,
      const std::vector<image_file_attribute> &in_vector
  );

 public:
  using base_type = process_t<image_to_move>;
  image_to_move();
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

  template <typename CompletionHandler>
  auto async_create_move(
      const entt::handle &in_handle,
      const std::vector<image_file_attribute> &in_vector,
      CompletionHandler &&in_completion
  ) {
    using l_call = std::function<void()>;
    in_handle.any_of<process_message>() ? void() : throw_exception(doodle_error{"缺失进度指示结构"});
    in_handle.any_of<FSys::path>() ? void() : throw_exception(doodle_error{"缺失输出文件路径"});
    std::for_each(
        std::begin(in_vector), std::end(in_vector), [](const image_file_attribute &in) {
          exists(in.file_path) ? void() : throw_exception(doodle_error{"找不到路径指向的文件"});
        }
    );
    in_vector.empty() ? void() : throw_exception(doodle_error{"没有传入任何的图片"});

    return boost::asio::async_initiate<CompletionHandler, void()>(
        [this, in_handle, in_vector](auto &&in_completion_handler) {
          auto l_f = std::make_shared<l_call>(
              std::forward<decltype(in_completion_handler)>(in_completion_handler)
          );
          boost::asio::post(
              [this, l_f, in_handle, in_vector]() {
                this->create_move(in_handle, in_vector);
                boost::asio::post(g_io_context(), [l_f]() { (*l_f)(); });
              }
          );
        },
        in_completion
    );
  };



  virtual ~image_to_move();
  [[maybe_unused]] void init();
  [[maybe_unused]] void succeeded();
  [[maybe_unused]] void failed();
  [[maybe_unused]] void aborted();
  [[maybe_unused]] void update(const base_type::delta_type &, void *data);
};
}  // namespace details
using image_to_move = details::image_to_move;
}  // namespace doodle
