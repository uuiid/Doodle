//
// Created by TD on 2023/11/17.
//
#include <doodle_core/doodle_core.h>

int core_error_enum(int, char** const) {
  using namespace doodle;
  boost::system::error_code l_ec{error_enum::file_not_exists};
  BOOST_ASIO_ERROR_LOCATION(l_ec);
}