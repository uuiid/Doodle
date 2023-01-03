#include "server.h"

#include "doodle_core/core/core_help_impl.h"
#include "doodle_core/doodle_core_fwd.h"
#include "doodle_core/logger/logger.h"
#include "doodle_core/metadata/metadata.h"
#include "doodle_core/metadata/user.h"
#include "doodle_core/metadata/work_task.h"

#include <boost/asio.hpp>
#include <boost/asio/post.hpp>
#include <boost/asio/strand.hpp>
#include <boost/system/system_error.hpp>

#include <azmq/message.hpp>
#include <azmq/socket.hpp>
#include <cstddef>
#include <fmt/core.h>
#include <functional>
#include <memory>
#include <tuple>
#include <vector>
#include <zmq.h>
#include <zmq.hpp>

namespace doodle::distributed_computing {

task::task() : socket_server() {}

void task::run_task() {
  socket_server = std::make_shared<zmq::socket_t>(g_reg()->ctx().emplace<zmq::context_t>(), zmq::socket_type::rep);

  register_fun_t("list_users"s, [this]() -> std::vector<std::tuple<database, user>> { return this->list_users(); });
  register_fun_t(
      "get_user_work_task_info"s,
      [this](
          const database& in_tocken, const database& in_user
      ) -> std::vector<std::tuple<database, doodle::work_task_info>> {
        return this->get_user_work_task_info(in_tocken.find_by_uuid(), in_user.find_by_uuid());
      }
  );
}

std::vector<std::tuple<database, doodle::user>> task::list_users() {
  std::vector<std::tuple<database, doodle::user>> l_r{};
  for (auto&& [e, d, u] : g_reg()->view<database, user>().each()) {
    l_r.emplace_back(d, u);
  }
  return l_r;
}
std::vector<std::tuple<database, doodle::work_task_info>> task::get_user_work_task_info(
    const entt::handle& in_tocken, const entt::handle& in_user
) {
  std::vector<std::tuple<database, doodle::work_task_info>> l_r{};
  // 当没有权限或者只查询自身时, 直接返回错误
  if (in_tocken.get<user>().power != power_enum::modify_other_users && in_tocken != in_user) {
    return l_r;
  }

  for (auto&& [e, d, u] : g_reg()->view<database, work_task_info>().each()) {
    if (u.user_ref.user_attr() == in_user) l_r.emplace_back(d, u);
  }
  return l_r;
}

server::server() : socket_frontend(), socket_backend(), socket_server() {}

void server::run() {
  socket_frontend = std::make_shared<zmq::socket_t>(g_reg()->ctx().emplace<zmq::context_t>(), zmq::socket_type::router);
  socket_backend  = std::make_shared<zmq::socket_t>(g_reg()->ctx().emplace<zmq::context_t>(), zmq::socket_type::dealer);
  socket_server   = std::make_shared<zmq::socket_t>(g_reg()->ctx().emplace<zmq::context_t>(), zmq::socket_type::rep);
  boost::asio::post(g_thread(), [this]() {
    socket_frontend->bind("tcp://*:23333");
    socket_backend->bind("tcp://*:23334");
    zmq_proxy(socket_frontend->handle(), socket_backend->handle(), nullptr);
  });
  boost::asio::post(g_thread(), [&]() { create_backend(); });
}

void server::create_backend() {
  // socket_server->bind("tcp://*:23333");
  socket_server->connect("tcp://127.0.0.1:23334");
  zmq::message_t l_msg{};
  auto l_r = socket_server->recv(l_msg);
  std::cout << l_msg.to_string() << std::endl;

  auto l_s = fmt::format("server: {}", l_msg.to_string_view());
  l_msg.rebuild(l_s.data(), l_s.size());
  std::cout << l_msg.to_string() << std::endl;

  socket_server->send(l_msg, zmq::send_flags::none);
}
}  // namespace doodle::distributed_computing