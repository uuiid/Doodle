//
// Created by td_main on 2023/8/28.
//
#include "client_core.h"

#include <doodle_core/lib_warp/boost_fmt_error.h>

#include <doodle_lib/core/bind_front_handler.h>

#include <boost/asio.hpp>
namespace doodle::detail {

void client_core::make_ptr() {
  auto l_s        = boost::asio::make_strand(g_io_context());
  ptr_->socket_   = std::make_shared<socket_t>(l_s);
  ptr_->resolver_ = std::make_shared<resolver_t>(l_s);
  ptr_->resolver_->async_resolve(
      ptr_->server_ip_, "50021", boost::beast::bind_front_handler(&client_core::on_resolve, this)
  );
  ptr_->executor_ = boost::asio::make_strand(g_io_context());
  ptr_->logger_   = g_logger_ctrl().make_log("client_core");
}
void client_core::on_resolve(boost::system::error_code ec, boost::asio::ip::tcp::resolver::results_type results) {
  if (ec) {
    DOODLE_LOG_ERROR("on_resolve error: {}", ec.message());
    return;
  }
  ptr_->resolver_results_ = std::move(results);
}
client_core::~client_core() = default;
void client_core::do_close() {
  boost::system::error_code ec;
  ptr_->socket_->socket().shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
  if (ec) {
    DOODLE_LOG_INFO(ec);
  }
}
void client_core::cancel() {
  boost::system::error_code ec;
  ptr_->socket_->socket().cancel(ec);
  if (ec) {
    DOODLE_LOG_INFO(ec);
  }
}
}  // namespace doodle::detail