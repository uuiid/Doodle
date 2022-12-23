#include "server.h"

#include "doodle_core/doodle_core_fwd.h"
#include "doodle_core/logger/logger.h"

#include <boost/asio/strand.hpp>
#include <boost/beast/http/message.hpp>
#include <boost/beast/http/string_body.hpp>
#include <boost/system/detail/error_code.hpp>
#include <boost/system/system_error.hpp>

#include "distributed_computing/server.h"
#include <azmq/message.hpp>
#include <azmq/socket.hpp>
#include <cstddef>
#include <zmq.h>

namespace doodle::distributed_computing {

server::server() : socket_frontend(g_io_context()), socket_backend(g_io_context()), socket_server(g_io_context()) {
  azmq::message_vector l_msgs{};
  boost::beast::http::request<boost::beast::http::string_body> l_m{};
  socket_frontend.bind("tcp://*:23333");
  socket_backend.bind("tcp://*:23334");
  socket_server.connect("tcp://*:23334");
}

void server::run() {
  create_backend();
  zmq_proxy(socket_frontend.native_handle(), socket_backend.native_handle(), nullptr);
}

void server::create_backend() {
  socket_server.async_receive([&](const boost::system::error_code& in_e, azmq::message& in_msg,
                                  std::size_t bytes_transferred) { DOODLE_LOG_ERROR(in_msg.string()); });
}
}  // namespace doodle::distributed_computing