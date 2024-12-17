//
// Created by TD on 2023/12/21.
//

#pragma once

#include "doodle_core/metadata/image_size.h"

#include <doodle_lib/doodle_lib_fwd.h>

namespace doodle::detail {

boost::system::error_code connect_video(
    const FSys::path &in_out_path, logger_ptr in_msg, const std::vector<FSys::path> &in_vector,
    const image_size &in_size
);

}  // namespace doodle::detail
