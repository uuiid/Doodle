//
// Created by td_main on 2023/10/18.
//
#include "logger_com.h"

namespace doodle {
socket_logger::socket_logger() {
  logger_ = g_logger_ctrl().make_log(fmt::format("socket {}", entt::to_entity(*g_reg(), *this)));
}
}  // namespace doodle::render_farm