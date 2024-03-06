//
// Created by TD on 2024/3/6.
//

#include "http_client_core.h"
namespace doodle::http::detail {

void client_core::make_ptr() {
  auto l_s        = boost::asio::make_strand(g_io_context());
  ptr_->socket_   = std::make_shared<socket_t>(l_s);
  ptr_->resolver_ = std::make_shared<resolver_t>(l_s);
  ptr_->resolver_->async_resolve(
      ptr_->server_ip_, std::to_string(doodle_config::http_port),
      boost::beast::bind_front_handler(&client_core::on_resolve, this)
  );
  ptr_->executor_ = boost::asio::make_strand(g_io_context());
  ptr_->logger_   = g_logger_ctrl().make_log(
      fmt::format("{} {} {} {}", typeid(*this).name(), fmt::ptr(this), ptr_->server_ip_, ptr_->socket_->socket())
  );
}
void client_core::on_resolve(boost::system::error_code ec, boost::asio::ip::tcp::resolver::results_type results) {
  if (ec) {
    DOODLE_LOG_ERROR("on_resolve error: {}", ec.message());
    return;
  }
  ptr_->resolver_results_ = std::move(results);
}
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
client_core::~client_core() = default;
}  // namespace doodle::http::detail