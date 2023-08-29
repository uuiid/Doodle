//
// Created by td_main on 2023/8/28.
//
#include "client_core.h"

#include <doodle_core/lib_warp/boost_fmt_error.h>

#include <doodle_lib/core/bind_front_handler.h>

#include <boost/asio.hpp>
namespace doodle::detail {

client_core::queue_action_guard::~queue_action_guard() {
  ptr_->queue_running_ = false;
  ptr_->queue_.pop();
  if (!ptr_->queue_.empty()) {
    ptr_->queue_.front()();
  }
}

void client_core::make_ptr() {
  auto l_s        = boost::asio::make_strand(g_io_context());
  ptr_->socket_   = std::make_shared<socket_t>(l_s);
  ptr_->resolver_ = std::make_shared<resolver_t>(l_s);
  //  ptr_->signal_set_ = std::make_shared<signal_set>(g_io_context(), SIGINT, SIGTERM);
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
  ptr_->queue_ = {};
}
}  // namespace doodle::detail