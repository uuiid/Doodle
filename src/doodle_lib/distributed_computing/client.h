

#pragma once

#include "doodle_lib/configure/doodle_lib_export.h"

#include <azmq/socket.hpp>
#include <string>
#include <zmq.hpp>
#include <zmq_addon.hpp>
namespace doodle::distributed_computing {
class DOODLELIB_API client {
  zmq::socket_t socket;

 public:
  client();

  void call(const std::string& in);
};
}  // namespace doodle::distributed_computing