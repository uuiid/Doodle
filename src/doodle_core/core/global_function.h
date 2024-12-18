//
// Created by TD on 2023/2/10.
//

#pragma once
#include <doodle_core/configure/config.h>
#include <doodle_core/configure/doodle_core_export.h>

#include <boost/asio.hpp>

#include <entt/entt.hpp>
namespace boost::asio {
class io_context;
class thread_pool;
}  // namespace boost::asio

namespace doodle {

namespace details {
class logger_ctrl;
}  // namespace details

DOODLE_CORE_API boost::asio::io_context &g_io_context();
DOODLE_CORE_API boost::asio::thread_pool &g_thread();
DOODLE_CORE_API details::logger_ctrl &g_logger_ctrl();

DOODLE_CORE_API boost::asio::strand<boost::asio::io_context::executor_type> &g_strand();
template <class... Ts>
struct overloaded : Ts... {
  using Ts::operator()...;
};
// explicit deduction guide (not needed as of C++20)
template <class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

}  // namespace doodle
