//
// Created by TD on 2023/12/21.
//

#pragma once

#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/metadata/move_create.h>
#include <doodle_core/thread_pool/process_message.h>

#include <boost/asio/async_result.hpp>
#include <boost/asio/post.hpp>
#include <boost/asio/thread_pool.hpp>

namespace doodle {
namespace detail {

/// 连接视屏
class DOODLE_CORE_API connect_video_interface {};

}  // namespace detail
using connect_video = std::shared_ptr<detail::connect_video_interface>;

}  // namespace doodle