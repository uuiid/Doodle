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
};
}  // namespace detail

using image_to_move = std::shared_ptr<detail::image_to_movie_interface>;
}  // namespace doodle
