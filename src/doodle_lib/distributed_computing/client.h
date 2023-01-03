

#pragma once

#include "doodle_core/json_rpc/core/rpc_client.h"

#include "doodle_lib/configure/doodle_lib_export.h"

#include <azmq/socket.hpp>
#include <entt/entity/fwd.hpp>
#include <string>
#include <vector>
#include <zmq.hpp>
#include <zmq_addon.hpp>

namespace doodle::distributed_computing {
class DOODLELIB_API client : public doodle::json_rpc::rpc_client {
  zmq::socket_t socket;

 public:
  client();

  std::vector<entt::handle> list_users();

  std::vector<entt::handle> get_user_work_task_info(const entt::handle& in_token, const entt::handle& in_user);

  void close();

 private:
  std::string call_server(const std::string& in_string, bool is_notice) override;
};
}  // namespace doodle::distributed_computing