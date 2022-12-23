

#pragma once

#include "doodle_lib/configure/doodle_lib_export.h"

#include <azmq/actor.hpp>
#include <azmq/context.hpp>
#include <azmq/message.hpp>
#include <azmq/socket.hpp>
#include <string>

namespace doodle::distributed_computing {

class DOODLELIB_API server {
  /// 请求
  azmq::dealer_socket socket_frontend;
  /// 后端
  azmq::router_socket socket_backend;
  /// 工作组
  azmq::rep_socket socket_server;

 public:
  server();

  void run();

 private:
  void create_backend();
};

}  // namespace doodle::distributed_computing