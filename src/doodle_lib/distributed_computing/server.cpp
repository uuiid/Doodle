#include "server.h"

#include "doodle_core/doodle_core_fwd.h"
#include "doodle_core/logger/logger.h"

#include <boost/asio.hpp>
#include <boost/asio/post.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/http/message.hpp>
#include <boost/beast/http/string_body.hpp>
#include <boost/system/detail/error_code.hpp>
#include <boost/system/system_error.hpp>

#include "distributed_computing/server.h"
#include <azmq/message.hpp>
#include <azmq/socket.hpp>
#include <cstddef>
#include <memory>
#include <vector>
#include <zmq.h>

namespace doodle::distributed_computing {

server::server() : socket_frontend(), socket_backend(), socket_server(g_io_context()) {}

void server::run() {
  create_backend();
  boost::asio::post(g_thread(), [this]() {
    socket_frontend = std::make_shared<azmq::dealer_socket>(g_io_context());
    socket_backend  = std::make_shared<azmq::router_socket>(g_io_context());
    socket_frontend->bind("tcp://*:23333");
    socket_backend->bind("tcp://*:23334");
    zmq_proxy(socket_frontend->native_handle(), socket_backend->native_handle(), nullptr);
  });
}

void server::create_backend() {
  socket_server.connect("tcp://*:23334");
  socket_server.async_receive([&](const boost::system::error_code& in_e, azmq::message& in_msg,
                                  std::size_t bytes_transferred) { DOODLE_LOG_ERROR(in_msg.string()); });
}
}  // namespace doodle::distributed_computing