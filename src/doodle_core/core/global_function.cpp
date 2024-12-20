#include "global_function.h"

#include <boost/exception/diagnostic_information.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/uuid/uuid_io.hpp>
namespace doodle {

tl::expected<boost::uuids::uuid, std::string> from_uuid_str(const std::string &uuid_str) {
  try {
    boost::uuids::uuid uuid;
    std::istringstream l_uuid_stream(uuid_str);
    l_uuid_stream >> uuid;
    if (l_uuid_stream.fail()) return tl::make_unexpected("Invalid uuid string");
    return uuid;
  } catch (...) {
    return tl::make_unexpected(boost::current_exception_diagnostic_information());
  }
}

}  // namespace doodle
