//
// Created by TD on 2024/2/20.
//

#include "socket_logger.h"
namespace doodle::http {
socket_logger::socket_logger() {
  logger_ = g_logger_ctrl().make_log(fmt::format("socket {}", entt::to_entity(*g_reg(), *this)));
}
}  // namespace doodle::http