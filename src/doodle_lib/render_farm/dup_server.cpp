//
// Created by td_main on 2023/9/1.
//

#include "dup_server.h"

namespace doodle {
void dup_server::run() {
  socket_.async_receive_from(buffer_, end_point_, [this](boost::system::error_code ec, std::size_t bytes) {
    if (ec) {
      return;
    }
    do_accept();
  });
}
void dup_server::do_accept() {
  std::string_view l_view{boost::asio::buffer_cast<const char *>(buffer_.data()), buffer_.size()};
  if (l_view == "hello world! doodle") {
    socket_.async_send_to(
        boost::asio::buffer("hello world! doodle"), end_point_,
        [this](boost::system::error_code ec, std::size_t bytes) { this->run(); }
    );
  }
}
}  // namespace doodle