

#pragma once

#include "doodle_core/doodle_core_fwd.h"
#include "doodle_core/json_rpc/core/rpc_server.h"
#include "doodle_core/metadata/metadata.h"

#include "doodle_lib/configure/doodle_lib_export.h"

#include <entt/entity/fwd.hpp>
#include <memory>
#include <string>
#include <zmq.hpp>
#include <zmq_addon.hpp>


namespace doodle::distributed_computing {

class task : public doodle::json_rpc::rpc_server {
  /// 工作组
  std::shared_ptr<zmq::socket_t> socket_server;

 public:
  task();

  void run_task();

 protected:
  std::vector<std::tuple<database, doodle::user>> list_users();
  std::vector<std::tuple<database, doodle::work_task_info>> get_user_work_task_info(
      const entt::handle& in_tocken, const entt::handle& in_user
  );
};

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