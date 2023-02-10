//
// Created by TD on 2023/2/10.
//

#pragma once
#include <doodle_core/configure/config.h>
#include <doodle_core/configure/doodle_core_export.h>

#include <entt/entt.hpp>
namespace boost::asio {
class io_context;
class thread_pool;
}  // namespace boost::asio

namespace doodle {

namespace details {
class database_info;
class program_info;
class identifier;
class logger_ctrl;
}  // namespace details

using registry_ptr = std::shared_ptr<entt::registry>;
DOODLE_CORE_API boost::asio::io_context &g_io_context();
DOODLE_CORE_API boost::asio::thread_pool &g_thread();
DOODLE_CORE_API registry_ptr &g_reg();
DOODLE_CORE_API registry_ptr &g_reg();
DOODLE_CORE_API details::logger_ctrl &g_logger_ctrl();
}  // namespace doodle
