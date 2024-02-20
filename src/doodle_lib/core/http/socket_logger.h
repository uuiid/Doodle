//
// Created by TD on 2024/2/20.
//

#pragma once
#include <doodle_lib/doodle_lib_fwd.h>
namespace doodle::http {
struct socket_logger {
  socket_logger();
  logger_ptr logger_{};
};
}  // namespace doodle::http