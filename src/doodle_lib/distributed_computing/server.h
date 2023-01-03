

#pragma once

#include "doodle_lib/configure/doodle_lib_export.h"

#include <memory>
#include <string>
#include <zmq.hpp>
#include <zmq_addon.hpp>

namespace doodle::distributed_computing {

class DOODLELIB_API server {
  /// 请求(异步)
  std::shared_ptr<zmq::socket_t> socket_backend;
  /// 后端(路由)
  std::shared_ptr<zmq::socket_t> socket_frontend;
  /// 工作组
  std::shared_ptr<zmq::socket_t> socket_server;
  
 public:
  server();

  void run();

 private:
  void create_backend();
};

}  // namespace doodle::distributed_computing