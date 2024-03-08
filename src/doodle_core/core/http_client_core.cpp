//
// Created by TD on 2024/3/6.
//

#include "http_client_core.h"
namespace doodle::http::detail {

void http_client_core::make_ptr() {
  auto l_s        = boost::asio::make_strand(g_io_context());
  ptr_->socket_   = std::make_shared<socket_t>(l_s);
  ptr_->resolver_ = std::make_shared<resolver_t>(l_s);
  ptr_->logger_ =
      g_logger_ctrl().make_log(fmt::format("{}_{}_{}", "http_client_core", ptr_->server_ip_, ptr_->socket_->socket()));
}
void http_client_core::next() {
  if (ptr_->next_list_.empty()) {
    return;
  }
  if (is_run_) return;
  ptr_->next_list_.front()->run();
  ptr_->next_list_.pop();
}

void http_client_core::do_close() {
  boost::system::error_code ec;
  ptr_->socket_->socket().shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
  if (ec) {
    DOODLE_LOG_INFO(ec);
  }
}
void http_client_core::cancel() {
  boost::system::error_code ec;
  ptr_->socket_->socket().cancel(ec);
  if (ec) {
    DOODLE_LOG_INFO(ec);
  }
}
http_client_core::~http_client_core() = default;
}  // namespace doodle::http::detail