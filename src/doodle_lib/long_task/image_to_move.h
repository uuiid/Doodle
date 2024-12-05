//
// Created by TD on 2021/12/27.
//
#pragma once

#include <doodle_core/metadata/move_create.h>
#include <doodle_core/thread_pool/image_to_movie.h>

#include <doodle_lib/doodle_lib_fwd.h>

#include <boost/asio.hpp>

#include <opencv2/core/types.hpp>

namespace doodle {
namespace detail {
FSys::path create_out_path(
    const FSys::path& in_dir, const episodes& in_eps, const shot& in_shot, const project* in_project = nullptr
);
boost::system::error_code create_move(
    const FSys::path& in_out_path, logger_ptr in_msg, const std::vector<movie::image_attr>& in_vector
);

}  // namespace detail
}  // namespace doodle