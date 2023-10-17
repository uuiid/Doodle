//
// Created by td_main on 2023/9/20.
//

#pragma once

namespace doodle::render_farm {
struct socket_logger {
  socket_logger() { logger_ = g_logger_ctrl().make_log(fmt::format("socket {}", entt::to_entity(*g_reg(), *this))); }
  logger_ptr logger_{};
};

}  // namespace doodle::render_farm