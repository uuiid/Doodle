#include "global_function.h"

#include <boost/exception/diagnostic_information.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/uuid/uuid_io.hpp>

#include "doodle_core/exception/exception.h"
namespace doodle {

boost::uuids::uuid from_uuid_str(const std::string& uuid_str) {
  boost::uuids::uuid uuid;
  std::istringstream l_uuid_stream(uuid_str);
  l_uuid_stream >> uuid;
  if (l_uuid_stream.fail()) throw std::runtime_error{fmt::format("Invalid UUID: {}", uuid_str)};
  return uuid;
}
std::chrono::time_zone<std::chrono::system_clock::duration> from_chrono_time_zone_str(
    const std::string& in_time_zone_str
) {
  std::chrono::time_zone<std::chrono::system_clock> l_time_zone;
  std::istringstream l_time_zone_stream(in_time_zone_str);
  using time_point = std::chrono::zoned_time<std::chrono::system_clock::duration>;

  if (std::chrono::time_point<std::chrono::system_clock> l_time{};
      l_time_zone_stream >> std::chrono::parse("%FT%TZ", l_time))
    l_time_zone = time_point{std::chrono::current_zone(), l_time};
  else if (l_time_zone_stream.clear(), l_time_zone_stream.str(in_time_zone_str),
           l_time_zone_stream >> std::chrono::parse("%FT%T%Ez", l_time))
    l_time_zone = time_point{std::chrono::current_zone(), l_time};
  else if (std::chrono::time_point<std::chrono::local_t, std::chrono::system_clock::duration> l_loc_time{};
           l_time_zone_stream.clear(), l_time_zone_stream.str(in_time_zone_str),
           l_time_zone_stream >> std::chrono::parse("%F %T", l_loc_time))
    l_time_zone = time_point{std::chrono::current_zone(), l_loc_time};
  else if (l_time_zone_stream.clear(), l_time_zone_stream.exceptions(std::ios::failbit),
           l_time_zone_stream.str(in_time_zone_str), l_time_zone_stream >> std::chrono::parse("%F", l_loc_time))
    l_time_zone = time_point{std::chrono::current_zone(), l_loc_time};
  else
    throw_exception(doodle_error{"Invalid time zone: {}", in_time_zone_str});
  return l_time_zone;
}

}  // namespace doodle
