#include "client.h"

#include "doodle_core/core/core_help_impl.h"
#include "doodle_core/doodle_core_fwd.h"
#include "doodle_core/logger/logger.h"
#include "doodle_core/metadata/metadata.h"
#include "doodle_core/metadata/user.h"
#include "doodle_core/metadata/work_task.h"

#include <boost/asio.hpp>
#include <boost/asio/post.hpp>
#include <boost/system/detail/error_code.hpp>

#include "distributed_computing/client.h"
#include <azmq/message.hpp>
#include <cstddef>
#include <entt/entity/fwd.hpp>
#include <iostream>
#include <string>
#include <tuple>
#include <vector>
#include <zmq.hpp>

namespace doodle::distributed_computing {

client::client() : socket(g_reg()->ctx().emplace<zmq::context_t>(), zmq::socket_type::req) {
  socket.connect("tcp://127.0.0.1:23333");
}

std::string client::call_server(const std::string& in_string, bool is_notice) {
  socket.send(zmq::message_t{in_string}, zmq::send_flags::none);
  zmq::message_t l_msg;
  auto l_r = socket.recv(l_msg);
  return l_msg.to_string();
}

std::vector<entt::handle> client::list_users() {
  auto l_user = call_fun<std::vector<std::tuple<database, user>>>("list_users"s);

  std::vector<entt::handle> l_r{};
  for (auto&& [data, l_u] : l_user) {
    if (auto l_user = data.find_by_uuid()) {
      l_user.emplace_or_replace<user>(l_u);
      l_r.emplace_back(l_user);
    } else {
      l_user = make_handle();
      l_user.emplace<database>(data);
      l_user.emplace<user>(l_u);
      l_r.emplace_back(l_user);
    }
  };
  return l_r;
}

std::vector<entt::handle> client::get_user_work_task_info(const entt::handle& in_token, const entt::handle& in_user) {
  auto l_user = call_fun<std::vector<std::tuple<database, work_task_info>>>(
      "get_user_work_task_info"s, std::make_tuple(in_token.get<database>(), in_user.get<database>())
  );

  std::vector<entt::handle> l_r{};
  for (auto&& [data, l_u] : l_user) {
    if (auto l_user = data.find_by_uuid()) {
      l_user.emplace_or_replace<work_task_info>(l_u);
      l_r.emplace_back(l_user);
    } else {
      l_user = make_handle();
      l_user.emplace<database>(data);
      l_user.emplace<work_task_info>(l_u);
      l_r.emplace_back(l_user);
    }
  };
  return l_r;
}

void client::close() { call_fun<void, true>("rpc.close"s); }
}  // namespace doodle::distributed_computing