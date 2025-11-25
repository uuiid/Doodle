//
// Created by TD on 2024/2/20.
//

#include "http_listener.h"

#include "doodle_core/core/core_set.h"
#include "doodle_core/doodle_core_fwd.h"
#include <doodle_core/core/app_base.h>

#include <doodle_lib/core/http/http_route.h>
#include <doodle_lib/core/http/http_session_data.h>

#include <boost/asio/bind_cancellation_slot.hpp>

namespace doodle::http {
boost::asio::awaitable<void> detail::run_http_listener(
    boost::asio::io_context& in_io_context, http_route_ptr in_route_ptr, std::uint16_t in_port
) {
  using executor_type = boost::asio::as_tuple_t<boost::asio::use_awaitable_t<>>;
  using endpoint_type = boost::asio::ip::tcp::endpoint;
  using acceptor_type = executor_type::as_default_on_t<boost::asio::ip::tcp::acceptor>;

  endpoint_type l_end_point{boost::asio::ip::tcp::v4(), in_port};
  auto l_executor = in_io_context.get_executor();
  try {
    acceptor_type l_acceptor{
        co_await boost::asio::this_coro::executor,
        l_end_point,
    };
    l_acceptor.listen(boost::asio::socket_base::max_listen_connections);
    auto l_local_endpoint = l_acceptor.local_endpoint();
    std::cout << l_local_endpoint.port() << std::endl;
    while ((co_await boost::asio::this_coro::cancellation_state).cancelled() == boost::asio::cancellation_type::none) {
      auto [l_ec, l_socket] = co_await l_acceptor.async_accept();
      if (l_ec) {
        if (l_ec == boost::asio::error::operation_aborted) {
          continue;
        }
        default_logger_raw()->log(log_loc(), level::err, "on_accept error: {}", l_ec.message());
      } else {
        std::make_shared<session_data>(std::move(l_socket), in_route_ptr)->async_run_detached();
      }
    }
  } catch (...) {
    default_logger_raw()->log(log_loc(), level::err, boost::current_exception_diagnostic_information());
    app_base::Get().stop_app(1);
  }
}
void run_http_listener(boost::asio::io_context& in_io_context, http_route_ptr in_route_ptr, std::uint16_t in_port) {
  g_ctx().emplace<detail::http_listener_cancellation_slot>();
  boost::asio::co_spawn(
      g_io_context(), detail::run_http_listener(in_io_context, in_route_ptr, in_port),
      boost::asio::bind_cancellation_slot(
          g_ctx().get<detail::http_listener_cancellation_slot>().slot(), boost::asio::detached
      )
  );
}
}  // namespace doodle::http