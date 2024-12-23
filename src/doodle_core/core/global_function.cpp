#include "global_function.h"

#include <boost/exception/diagnostic_information.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/uuid/uuid_io.hpp>
namespace doodle {

boost::uuids::uuid from_uuid_str(const std::string &uuid_str) {
  boost::uuids::uuid uuid;
  std::istringstream l_uuid_stream(uuid_str);
  l_uuid_stream >> uuid;
  if (l_uuid_stream.fail()) throw std::runtime_error{fmt::format("Invalid UUID: {}", uuid_str)};
  return uuid;
}

}  // namespace doodle
