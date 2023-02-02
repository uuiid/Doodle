#include "client.h"

#include "doodle_core/core/core_help_impl.h"
#include "doodle_core/doodle_core_fwd.h"
#include "doodle_core/exception/exception.h"
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

client::client(const std::string& in_server_ip) : client(g_reg(), in_server_ip) {
  socket.connect(fmt::format("tcp://{}:23333", in_server_ip));
}
client::client(const registry_ptr& in_reg, const std::string& in_server_ip)
    : reg(in_reg), socket(in_reg->ctx().emplace<zmq::context_t>(), zmq::socket_type::req) {
  socket.connect(fmt::format("tcp://{}:23333", in_server_ip));
}

std::string client::call_server(const std::string& in_string, bool is_notice) {
  socket.send(zmq::message_t{in_string}, zmq::send_flags::none);
  zmq::message_t l_msg;
  if (is_stop) {
    return {};
  }
  auto l_r = socket.recv(l_msg);
  return l_msg.to_string();
}

std::vector<std::string> client::list_fun() { return call_fun<std::vector<std::string>>("rpc.list_fun"s); }

std::vector<entt::handle> client::list_users() {
  auto l_user = call_fun<std::vector<std::tuple<entt::entity, user>>>("list.user"s);

  std::vector<entt::handle> l_r{};
  for (auto&& [l_h, l_u] : l_user) {
    auto l_r_h = entt::handle{*reg, l_h};
    if (!l_r_h) {
      l_r_h = entt::handle{*reg, reg->create(l_h)};
    }
    l_r_h.emplace_or_replace<user>(l_u);
    l_r.emplace_back(l_r_h);
  };
  return l_r;
}

entt::handle client::set_user(const entt::handle& in_user) {
  if (!in_user) throw_exception(doodle_error{"无效的句柄"});
  if (!reg->ctx().contains<user::current_user>()) throw_exception(doodle_error{"没有当前用户"});
  auto l_current_user = reg->ctx().at<user::current_user>().get_handle();

  auto l_data         = call_fun<database>(
      "set.user"s, std::make_tuple(l_current_user.get<database>(), in_user.entity(), in_user.get<user>())
  );
  in_user.emplace_or_replace<database>(l_data);

  return in_user;
}
entt::handle client::new_user(const user& in_user) {
  auto&& [e, l_data] = call_fun<std::tuple<entt::entity, database>>("new.user"s, in_user);
  auto l_h           = entt::handle{*reg, reg->create(e)};
  l_h.emplace<database>(l_data);
  l_h.emplace<user>(in_user);
  return l_h;
}
entt::handle client::get_user(const boost::uuids::uuid& in_user) {
  auto [l_e, l_u, l_d] = call_fun<std::tuple<entt::entity, user, database>>("get.user"s, in_user);

  auto l_h             = entt::handle{*reg, reg->valid(l_e) ? l_e : reg->create(l_e)};
  l_h.emplace<database>(l_d);
  l_h.emplace<user>(l_u);
  return l_h;
}

std::vector<entt::handle> client::get_user_work_task_info(const entt::handle& in_token, const entt::handle& in_user) {
  auto l_user = call_fun<std::vector<std::tuple<entt::entity, work_task_info>>>(
      "get.filter.work_task_info"s, std::make_tuple(in_token.get<database>(), in_user.entity())
  );

  std::vector<entt::handle> l_r{};
  for (auto&& [l_h, l_u] : l_user) {
    auto l_r_h = entt::handle{*reg, l_h};
    if (!l_r_h) {
      l_r_h = entt::handle{*reg, reg->create(l_h)};
    }
    l_r_h.emplace_or_replace<work_task_info>(l_u);
    l_r.emplace_back(l_r_h);
  };
  return l_r;
}

entt::handle client::set_work_task_info(const entt::handle& in_token, const entt::handle& in_work) {
  if (!reg->ctx().contains<user::current_user>()) throw_exception(doodle_error{"没有当前用户"});
  if (!in_work.all_of<work_task_info>()) throw_exception(doodle_error{"缺失组件"});

  auto l_current_user = reg->ctx().at<user::current_user>().get_handle();
  call_fun<void, false>(
      "set.work_task_info"s, std::make_tuple(in_token.get<database>(), in_work.entity(), in_work.get<work_task_info>())
  );

  return in_work;
}

void client::destroy_entity(const entt::handle& in_entt) {
  if (!in_entt) throw_exception(doodle_error{"无效的句柄"});

  call_fun<void, false>("destroy.entity"s, std::make_tuple(in_entt.entity()));
}

void client::close() {
  is_stop = true;
  call_fun<void, true>("rpc.close"s);
}
}  // namespace doodle::distributed_computing