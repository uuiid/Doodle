//
// Created by TD on 2023/12/21.
//

#pragma once

#include <doodle_core/metadata/move_create.h>
#include <doodle_core/thread_pool/connect_video_interface.h>

#include <doodle_lib/doodle_lib_fwd.h>

#include <boost/asio.hpp>

#include <opencv2/core/types.hpp>

namespace doodle::detail {

class DOODLELIB_API connect_video_t : public connect_video_interface {
 private:
  /**
   * 使用句柄检查路径
   * 必须具有组件 FSys::path,和后缀 mp4
   * 可选的组件 ep,和shot, 可以用来添加标记
   * @param in_handle 传入的句柄
   * @return 规范的路径
   */
  FSys::path create_out_path(const entt::handle &in_handle) override;

 public:
  connect_video_t()          = default;
  virtual ~connect_video_t() = default;

  virtual boost::system::error_code connect_video(
      const FSys::path &in_out_path, logger_ptr in_msg, const std::vector<FSys::path> &in_vector
  ) override;
};

}  // namespace doodle::detail
