//
// Created by td_main on 2023/9/20.
//

#pragma once
#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/logger/logger.h>

namespace doodle {
namespace render_farm {
class working_machine_session;
class working_machine;
namespace detail {
struct basic_json_body;
class http_route;
class ue4_task;
class render_ue4;

}  // namespace detail
using render_ue4     = detail::render_ue4;
using render_ue4_ptr = std::shared_ptr<render_ue4>;
using http_route_ptr = std::shared_ptr<detail::http_route>;
struct socket_logger {
  socket_logger() { logger_ = g_logger_ctrl().make_log(fmt::format("socket {}", entt::to_entity(*g_reg(), *this))); }
  logger_ptr logger_{};
};
}  // namespace render_farm
}  // namespace doodle
