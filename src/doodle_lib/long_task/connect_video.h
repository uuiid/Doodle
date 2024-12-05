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


boost::system::error_code connect_video(
    const FSys::path &in_out_path, logger_ptr in_msg, const std::vector<FSys::path> &in_vector
  );

}  // namespace doodle::detail
