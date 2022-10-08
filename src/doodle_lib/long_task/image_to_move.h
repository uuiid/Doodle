//
// Created by TD on 2021/12/27.
//
#pragma once

#include <doodle_lib/doodle_lib_fwd.h>
#include <opencv2/core/types.hpp>

#include <boost/asio.hpp>
#include <boost/asio.hpp>

namespace doodle {
class DOODLELIB_API image_watermark {
 public:
  image_watermark() = default;
  image_watermark(std::string in_p_text, double_t in_p_width_proportion, double_t in_p_height_proportion, cv::Scalar in_rgba);
  std::string p_text{};
  std::double_t p_width_proportion{};
  std::double_t p_height_proportion{};
  cv::Scalar rgba{};
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

  virtual void create_move(
      const FSys::path &in_out_path,
      process_message &in_msg,
      const std::vector<image_file_attribute> &in_vector
  );
  FSys::path create_out_path(const entt::handle &in_handle);

  static std::vector<image_file_attribute> make_default_attr(
      const entt::handle &in_handle,
      const std::vector<FSys::path> &in_path_list
  );

 public:
  image_to_move();
  virtual ~image_to_move();

  inline static const cv::Scalar rgb_default{25, 220, 2};

  template <typename CompletionHandler>
  auto async_create_move(
      const entt::handle &in_handle,
      const std::vector<FSys::path> &in_vector,
      CompletionHandler &&in_completion
  ) {
    return async_create_move(
        in_handle,
        make_default_attr(in_handle, in_vector),
        std::forward<decltype(in_completion)>(in_completion)
    );
  }

  template <typename CompletionHandler>
  auto async_create_move(
      const entt::handle &in_handle,
      const std::vector<image_file_attribute> &in_vector,
      CompletionHandler &&in_completion
  ) {
    using l_call = std::function<void()>;
    in_handle.any_of<FSys::path>() ? void() : throw_exception(doodle_error{"缺失输出文件路径"});
    std::for_each(
        std::begin(in_vector), std::end(in_vector), [](const image_file_attribute &in) {
          exists(in.file_path) ? void() : throw_exception(doodle_error{"找不到路径指向的文件"});
        }
    );
    !in_vector.empty() ? void() : throw_exception(doodle_error{"没有传入任何的图片"});
    auto l_msg_ref  = std::ref(in_handle.get_or_emplace<process_message>());
    auto l_out_path = this->create_out_path(in_handle);
    return boost::asio::async_initiate<CompletionHandler, void()>(
        [this,
         in_vector,
         l_msg_ref,
         l_out_path = std::move(l_out_path)](auto &&in_completion_handler) {
          auto l_f = std::make_shared<l_call>(
              std::forward<decltype(in_completion_handler)>(in_completion_handler)
          );
          boost::asio::post(
              g_thread(),
              [this,
               l_f,
               in_vector,
               l_msg_ref,
               l_out_path]() {
                this->create_move(l_out_path, l_msg_ref, in_vector);
                boost::asio::post(g_io_context(), [l_f]() { (*l_f)(); });
              }
          );
        },
        in_completion
    );
  };
};
}  // namespace details
using image_to_move = std::shared_ptr<details::image_to_move>;
}  // namespace doodle
