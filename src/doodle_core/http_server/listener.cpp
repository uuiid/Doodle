//
// Created by TD on 2022/8/26.
//

#include "listener.h"
#include <doodle_core/http_server/session.h>
namespace doodle {
namespace http_server {
class listener::impl {
 public:
  boost::asio::ip::tcp::acceptor acceptor_{g_io_context()};
};
listener::listener(boost::asio::ip::tcp::endpoint in_end_point)
    : ptr(std::make_unique<impl>()) {
  // Open the acceptor
  ptr->acceptor_.open(in_end_point.protocol());

  // Allow address reuse
  ptr->acceptor_.set_option(boost::asio::socket_base::reuse_address(true));

  // Bind to the server address
  ptr->acceptor_.bind(in_end_point);

  // Start listening for connections
  ptr->acceptor_.listen(
      boost::asio::socket_base::max_listen_connections
  );
}
void listener::run() {
  do_accept();
}
void listener::do_accept() {
  // The new connection gets its own strand
  ptr->acceptor_.async_accept(
      boost::asio::make_strand(g_io_context()),
      boost::beast::bind_front_handler(
          &listener::on_accept,
          shared_from_this()
      )
  );
};

void listener::on_accept(boost::beast::error_code ec, boost::asio::ip::tcp::socket socket) {
  if (ec) {
    //      fail(ec, "accept");
    return;  // To avoid infinite loop
  } else {
    // Create the session and run it
    std::make_shared<session>(
        std::move(socket)
    )
        ->run();
  }
  // Accept another connection
  do_accept();
};
}  // namespace http_server
}  // namespace doodle
