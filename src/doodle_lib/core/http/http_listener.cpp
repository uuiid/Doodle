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

boost::asio::awaitable<void> detail::run_http_listener(
    boost::asio::io_context& in_io_context, http_route_ptr in_route_ptr, std::uint16_t in_port
) {
  using executor_type = boost::asio::as_tuple_t<boost::asio::use_awaitable_t<>>;
  using endpoint_type = boost::asio::ip::tcp::endpoint;
  using acceptor_type = executor_type::as_default_on_t<boost::asio::ip::tcp::acceptor>;

  endpoint_type l_end_point{boost::asio::ip::tcp::v4(), in_port};
  auto l_executor = in_io_context.get_executor();

  acceptor_type l_acceptor{
      co_await boost::asio::this_coro::executor,
      l_end_point,
  };

  l_acceptor.listen(boost::asio::socket_base::max_listen_connections);
  while ((co_await boost::asio::this_coro::cancellation_state).cancelled() == boost::asio::cancellation_type::none) {
    auto [l_ec, l_socket] = co_await l_acceptor.async_accept();
    if (l_ec) {
      if (l_ec == boost::asio::error::operation_aborted) {
        continue;
      }
      default_logger_raw()->log(log_loc(), level::err, "on_accept error: {}", l_ec.message());
    } else {
      std::make_shared<http_session_data>(std::move(l_socket), in_route_ptr)->rend_head();
    }
  }
}
}  // namespace doodle::http