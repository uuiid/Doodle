//
// Created by TD on 2024/2/20.
//

#include "http_listener.h"

#include <doodle_app/app/app_command.h>

#include <doodle_lib/core/http/http_route.h>
#include <doodle_lib/core/http/http_session_data.h>
#include <doodle_lib/core/http/socket_logger.h>
namespace doodle::http {
void http_listener::run() {
  auto acceptor_ptr = std::make_shared<acceptor_type>(executor_, end_point_);
  acceptor_ptr->listen(boost::asio::socket_base::max_listen_connections);
  acceptor_ptr_ = std::move(acceptor_ptr);

  do_accept();
}
void http_listener::do_accept() {
  acceptor_ptr_->async_accept(
      boost::asio::make_strand(executor_),
      boost::asio::bind_cancellation_slot(
          app_base::Get().on_cancel.slot(), boost::beast::bind_front_handler(&http_listener::on_accept, this)
      )
  );
}
void http_listener::on_accept(boost::system::error_code ec, boost::asio::ip::tcp::socket socket) {
  if (ec) {
    if (ec == boost::asio::error::operation_aborted) {
      return;
    }
    default_logger_raw()->log(log_loc(), level::err, "on_accept error: {}", ec.message());
  } else {
    std::make_shared<http_session_data>(std::move(socket), route_ptr_)->rend_head();
  }
  do_accept();
}
}  // namespace doodle::http