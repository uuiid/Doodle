//
// Created by TD on 2023/2/10.
//

#pragma once
#include <doodle_core/configure/config.h>
#include <doodle_core/configure/doodle_core_export.h>

#include <boost/asio.hpp>
#include <boost/uuid/uuid.hpp>

#include <entt/entt.hpp>
#include <fmt/format.h>
#include <nlohmann/json.hpp>
#include <sstream>
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
DOODLE_CORE_API boost::asio::strand<boost::asio::io_context::executor_type> &g_pool_strand();
DOODLE_CORE_API std::size_t get_hardware_concurrency();
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

DOODLE_CORE_API boost::uuids::uuid from_uuid_str(const std::string &uuid_str);

template <typename Duration = std::chrono::system_clock::duration>
DOODLE_CORE_API std::chrono::zoned_time<std::chrono::system_clock::duration> from_chrono_time_zone_str(
    const std::string &in_time_zone_str
) {
  using time_point     = std::chrono::zoned_time<Duration>;
  using time_loc_point = std::chrono::time_point<std::chrono::local_t, Duration>;
  std::chrono::zoned_time<Duration> l_time_zone;
  std::istringstream l_stream(in_time_zone_str);
  const auto l_current_zone = std::chrono::current_zone();

  if (std::chrono::time_point<std::chrono::system_clock> l_time{}; l_stream >> std::chrono::parse("%FT%TZ", l_time))
    l_time_zone = time_point{l_current_zone, l_time};
  else if (l_stream.clear(), l_stream.str(in_time_zone_str), l_stream >> std::chrono::parse("%FT%T%Ez", l_time))
    l_time_zone = time_point{l_current_zone, l_time};
  else if (time_loc_point l_loc_time{};
           l_stream.clear(), l_stream.str(in_time_zone_str), l_stream >> std::chrono::parse("%F %T", l_loc_time))
    l_time_zone = time_point{l_current_zone, l_loc_time};
  else if (l_stream.clear(), l_stream.exceptions(std::ios::failbit), l_stream.str(in_time_zone_str),
           l_stream >> std::chrono::parse("%F", l_loc_time))
    l_time_zone = time_point{l_current_zone, l_loc_time};
  else
    throw std::runtime_error{fmt::format("Invalid time zone: {}", in_time_zone_str)};
  return l_time_zone;
};
template <typename T>
T parse_time(const std::string &time_str, const std::string &format) {
  std::istringstream l_year_month_stream{time_str};
  T l_time{};
  l_year_month_stream >> std::chrono::parse(format, l_time);
  if (!l_year_month_stream) throw std::runtime_error{fmt::format("Invalid time: {}", time_str)};
  return l_time;
}
}  // namespace doodle
