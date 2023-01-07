

#pragma once

#include "doodle_core/doodle_core_fwd.h"
#include "doodle_core/json_rpc/core/rpc_client.h"
#include "doodle_core/metadata/work_task.h"

#include "doodle_lib/configure/doodle_lib_export.h"
#include <boost/uuid/uuid.hpp>

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
  /// 列出所有注册的功能
  std::vector<std::string> list_fun();
  /// 列出所有的user
  std::vector<entt::handle> list_users();
  /// 设置user
  entt::handle set_user(const entt::handle& in_user);
  /// 创建新user
  entt::handle new_user(const user& in_user);
  /// 登录user
  entt::handle get_user(const boost::uuids::uuid& in_user);
  /// 删除user
  entt::handle delete_user(const entt::handle& in_user);

  /// 获取用户任务的信息
  std::vector<entt::handle> get_user_work_task_info(const entt::handle& in_token, const entt::handle& in_user);
  /// 设置任务信息(包括提交)
  entt::handle set_work_task_info(const entt::handle& in_token, const entt::handle& in_user);
  /// 删除任务信息
  entt::handle delete_work_task_info(const entt::handle& in_token, const entt::handle& in_user);

  void close();

 private:
  std::string call_server(const std::string& in_string, bool is_notice) override;
};
}  // namespace doodle::distributed_computing