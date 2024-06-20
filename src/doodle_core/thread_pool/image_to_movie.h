//
// Created by TD on 2022/10/10.
//

#pragma once

#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/metadata/move_create.h>
#include <doodle_core/thread_pool/process_message.h>

#include <boost/asio.hpp>
#include <boost/asio/async_result.hpp>
#include <boost/asio/post.hpp>
#include <boost/asio/thread_pool.hpp>
namespace doodle {
namespace detail {

class DOODLE_CORE_API image_to_movie_interface {
 public:
  /**
   * 测试和创建输出路径
   * @param in_handle 传入的句柄
   * @return 创建完成的路径
   */
  virtual FSys::path create_out_path(const entt::handle &in_handle) = 0;

 public:
  using image_attr      = ::doodle::movie::image_attr;
  using image_watermark = ::doodle::movie::image_watermark;
  struct out_file_path {
    FSys::path path;
  };

  image_to_movie_interface();
  virtual ~image_to_movie_interface();
  virtual boost::system::error_code create_move(
      const FSys::path &in_out_path, logger_ptr in_msg, const std::vector<image_attr> &in_vector
  ) = 0;

  template <typename CompletionHandler>
  auto async_create_move(
      const entt::handle &in_handle, const std::vector<FSys::path> &in_vector, CompletionHandler &&in_completion
  ) {
    return async_create_move(
        in_handle, image_attr::make_default_attr(in_handle, in_vector),
        std::forward<decltype(in_completion)>(in_completion)
    );
  }

  template <typename CompletionHandler>
  auto async_create_move(
      const entt::handle &in_handle, const std::vector<image_attr> &in_vector, CompletionHandler &&in_completion
  ) {
    in_handle.any_of<out_file_path>() ? void() : throw_exception(doodle_error{"缺失输出文件路径"});
    std::for_each(std::begin(in_vector), std::end(in_vector), [](const image_attr &in) {
      exists(in.path_attr) ? void() : throw_exception(doodle_error{"找不到路径指向的文件"});
    });
    !in_vector.empty() ? void() : throw_exception(doodle_error{"没有传入任何的图片"});
    if (!in_handle.all_of<process_message>())
      in_handle.emplace<process_message>(in_handle.get<out_file_path>().path.filename().string());
    auto l_logger   = in_handle.get<process_message>().logger();
    auto l_out_path = this->create_out_path(in_handle);
    return boost::asio::async_initiate<CompletionHandler, void(FSys::path, boost::system::error_code)>(
        [this, in_vector, l_out_path = std::move(l_out_path), in_handle, l_logger](auto &&in_completion_handler) {
          auto l_f = std::make_shared<std::decay_t<decltype(in_completion_handler)>>(
              std::forward<decltype(in_completion_handler)>(in_completion_handler)
          );
          boost::asio::post(g_thread(), [this, l_f, in_vector, l_out_path, in_handle, l_logger]() {
            auto l_ec = this->create_move(l_out_path, l_logger, in_vector);
            boost::asio::post(boost::asio::prepend(std::move(*l_f), l_out_path, l_ec));
          });
        },
        in_completion
    );
  };
};
}  // namespace detail

using image_to_move = std::shared_ptr<detail::image_to_movie_interface>;
}  // namespace doodle
