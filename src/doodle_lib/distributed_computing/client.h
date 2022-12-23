

#pragma once

#include "configure/doodle_lib_export.h"

#include <azmq/socket.hpp>
#include <string>

namespace doodle::distributed_computing {
class DOODLELIB_API client {
  azmq::req_socket socket;

 public:
  client();

  void call(const std::string& in);
};
}  // namespace doodle::distributed_computing