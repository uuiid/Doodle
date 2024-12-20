//
// Created by TD on 2023/2/10.
//

#pragma once
#include <doodle_core/configure/config.h>
#include <doodle_core/configure/doodle_core_export.h>

#include <boost/asio.hpp>
#include <boost/uuid/uuid.hpp>

#include <entt/entt.hpp>
#include <tl/expected.hpp>

namespace boost::asio {
class io_context;
class thread_pool;
}  // namespace boost::asio

namespace doodle {

namespace details {
class logger_ctrl;
}  // namespace details

DOODLE_CORE_API boost::asio::io_context &g_io_context();
DOODLE_CORE_API details::logger_ctrl &g_logger_ctrl();

DOODLE_CORE_API boost::asio::strand<boost::asio::io_context::executor_type> &g_strand();
template <class... Ts>
struct overloaded : Ts... {
  using Ts::operator()...;
};
// explicit deduction guide (not needed as of C++20)
template <class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;
/**
 * Converts a UUID string to a boost::uuids::uuid object.
 *
 * @param uuid_str The UUID string to be converted.
 * @return A tl::expected containing the boost::uuids::uuid object if successful,
 *         or an error message as a std::string if the conversion fails.
 * @note This function does not throw exceptions and uses noexcept.
 */

DOODLE_CORE_API tl::expected<boost::uuids::uuid, std::string> from_uuid_str(const std::string &uuid_str) noexcept;
}  // namespace doodle
