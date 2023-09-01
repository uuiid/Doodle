//
// Created by td_main on 2023/9/1.
//

#include "udp_server.h"

namespace doodle {
void udp_server::run() {
  socket_.async_receive_from(
      boost::asio::buffer(buffer_), end_point_,
      [this](boost::system::error_code ec, std::size_t bytes) {
        if (ec) {
          return;
        }
        do_accept();
      }
  );
}
void udp_server::do_accept() {
  std::string_view l_view{buffer_};
  if (l_view == "hello world! doodle") {
    socket_.async_send_to(
        boost::asio::buffer("hello world! doodle"), end_point_,
        [this](boost::system::error_code ec, std::size_t bytes) {
          buffer_.clear();
          this->run();
        }
    );
  }
}
}  // namespace doodle