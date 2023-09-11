//
// Created by td_main on 2023/9/1.
//

#include "udp_server.h"

#include "doodle_core/logger/logger.h"
#include <doodle_core/doodle_core_fwd.h>

namespace doodle {
void udp_server::run() {
  socket_.async_receive_from(
      boost::asio::buffer(buffer_), end_point_,
      [this](boost::system::error_code ec, std::size_t bytes) {
        if (ec == boost::asio::error::operation_aborted) {
          return;
        }
        if (ec) {
          log_error(logger_ptr_, fmt::format("async_receive_from error: {}", ec.message()));
          run();
          return;
        }
        do_accept(bytes);
      }
  );
}
void udp_server::do_accept(std::size_t in_size) {
  std::string_view l_view{buffer_, in_size};
  if (doodle_config::hello_world_doodle == l_view) {
    log_info(logger_ptr_, fmt::format("receive: {}", l_view));
    socket_.async_send_to(
        boost::asio::buffer(
            doodle_config::hello_world_doodle_server.data(), doodle_config::hello_world_doodle_server.size()
        ),
        end_point_,
        [this, in_size](boost::system::error_code ec, std::size_t bytes) {
          std::memset(buffer_, 0, in_size);
          this->run();
        }
    );
  } else {
    run();
  }
}
}  // namespace doodle