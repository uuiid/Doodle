#include "server.h"

#include "doodle_core/core/core_help_impl.h"
#include "doodle_core/doodle_core_fwd.h"
#include "doodle_core/logger/logger.h"

#include <boost/asio.hpp>
#include <boost/asio/post.hpp>
#include <boost/asio/strand.hpp>
#include <boost/system/system_error.hpp>

#include <azmq/message.hpp>
#include <azmq/socket.hpp>
#include <cstddef>
#include <fmt/core.h>
#include <memory>
#include <vector>
#include <zmq.h>
#include <zmq.hpp>

namespace doodle::distributed_computing {

server::server() : socket_frontend(), socket_backend(), socket_server() {}

void server::run() {
  socket_frontend = std::make_shared<zmq::socket_t>(g_reg()->ctx().emplace<zmq::context_t>(), zmq::socket_type::router);
  socket_backend  = std::make_shared<zmq::socket_t>(g_reg()->ctx().emplace<zmq::context_t>(), zmq::socket_type::dealer);
  boost::asio::post(g_thread(), [this]() {
    socket_frontend->bind("tcp://*:23333");
    socket_backend->bind("tcp://*:23334");
    zmq_proxy(socket_frontend->handle(), socket_backend->handle(), nullptr);
  });
  boost::asio::post(g_io_context(), [&]() { create_backend(); });
}

void server::create_backend() {
  socket_server = std::make_shared<zmq::socket_t>(g_reg()->ctx().emplace<zmq::context_t>(), zmq::socket_type::rep);
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