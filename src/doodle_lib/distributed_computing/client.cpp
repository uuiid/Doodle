#include "client.h"

#include "doodle_core/doodle_core_fwd.h"
#include "doodle_core/logger/logger.h"

#include <boost/system/detail/error_code.hpp>

#include "distributed_computing/client.h"
#include <azmq/message.hpp>
#include <cstddef>

namespace doodle::distributed_computing {

client::client() : socket(g_io_context()) { socket.connect("tcp://*:23333"); }

void client::call(const std::string& in) {
  socket.async_receive([](const boost::system::error_code& in, azmq::message& in_msg, std::size_t) {
    DOODLE_LOG_ERROR(in_msg.string());
  });

  socket.send(in);
}
}  // namespace doodle::distributed_computing