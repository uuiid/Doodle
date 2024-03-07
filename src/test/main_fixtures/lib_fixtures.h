

#pragma once

#include <doodle_core/doodle_core.h>
#include <doodle_core/metadata/metadata.h>
#include <doodle_core/metadata/time_point_wrap.h>
#include <doodle_core/metadata/user.h>

#include <boost/asio/io_context.hpp>

#include <memory>
#include <string>

struct lib_fixtures {
  lib_fixtures();
  virtual ~lib_fixtures();

  doodle::doodle_lib doodle_lib_attr{};
};

namespace doodle {
inline std::ostream& boost_test_print_type(std::ostream& ostr, database const& right) {
  ostr << "id: " << right.get_id() << " uuid: " << right.uuid();

  return ostr;
}
inline std::ostream& boost_test_print_type(std::ostream& ostr, user const& right) {
  ostr << "name: " << right.get_name() << "(" << right.get_enus() << ")";

  return ostr;
}
}  // namespace doodle