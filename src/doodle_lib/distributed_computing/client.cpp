#include "client.h"

#include "doodle_core/core/core_help_impl.h"
#include "doodle_core/doodle_core_fwd.h"
#include "doodle_core/logger/logger.h"

#include <boost/asio/post.hpp>
#include <boost/system/detail/error_code.hpp>

#include "distributed_computing/client.h"
#include <azmq/message.hpp>
#include <cstddef>
#include <iostream>
#include <zmq.hpp>

namespace doodle::distributed_computing {

client::client() : socket(g_reg()->ctx().emplace<zmq::context_t>(), zmq::socket_type::req) {
  socket.connect("tcp://127.0.0.1:23333");
}

void client::call(const std::string& in) {
  socket.send(zmq::message_t{in.data(), in.size()}, zmq::send_flags::none);
  boost::asio::post([&]() {
    zmq::message_t l_msg;
    auto l_r = socket.recv(l_msg);
    std::cout << l_msg.to_string() << std::endl;
  });
}
}  // namespace doodle::distributed_computing