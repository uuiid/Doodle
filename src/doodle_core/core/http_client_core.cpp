//
// Created by TD on 2024/3/6.
//

#include "http_client_core.h"

namespace doodle::http::detail {
void http_client_data_base::init(std::string in_server_url, boost::asio::ssl::context* in_ctx) {
  resolver_  = std::make_shared<resolver_t>(executor_);
  timer_ptr_ = std::make_shared<boost::asio::steady_timer>(executor_);
  boost::urls::url l_url{in_server_url};
  logger_    = g_logger_ctrl().make_log(fmt::format("{}_{}", "http_client", l_url.host()));
  server_ip_ = l_url.host();

  if (l_url.has_port())
    server_port_ = l_url.port();
  else if (l_url.scheme() == "https")
    server_port_ = "443";
  else
    server_port_ = "80";

  switch (l_url.scheme_id()) {
    case boost::urls::scheme::http: // http
      socket_ = std::make_shared<socket_t>(executor_);
      break;
    case boost::urls::scheme::https: // https
    {
      if (in_ctx == nullptr) {
        logger_->log(log_loc(), level::err, "https 需要 ssl context");
        return;
      }
      auto l_ssl = std::make_shared<ssl_socket_t>(executor_, *in_ctx);
      l_ssl->set_verify_mode(boost::asio::ssl::verify_none);
      if (!SSL_set_tlsext_host_name(l_ssl->native_handle(), server_ip_.c_str())) {
        boost::system::error_code ec{static_cast<int>(::ERR_get_error()), boost::asio::error::get_ssl_category()};
        logger_->log(log_loc(), level::err, "SSL_set_tlsext_host_name error: {}", ec.message());
        return;
      }
      socket_ = l_ssl;
    }
    break;
    default:
      logger_->log(log_loc(), level::err, "不支持的协议 {}", l_url.scheme());
      break;
  }
}

void http_client_data_base::do_close() {
  std::visit(
    entt::overloaded{
        [this](socket_ptr& in_socket) {
          boost::system::error_code ec;
          in_socket->socket().close();
          in_socket->socket().shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
          if (ec && ec != boost::beast::errc::not_connected) {
            logger_->log(log_loc(), level::err, "do_close error: {}", ec.message());
          }
          socket_ = {};
        },
        [this](ssl_socket_ptr& in_socket) {
          in_socket->async_shutdown([ptr = shared_from_this()](boost::system::error_code ec) {
            if (ec) {
              ptr->logger_->log(log_loc(), level::err, "do_close error: {}", ec.message());
            }
            ptr->socket_ = {};
          });
        }
    },
    socket_
  );

  // co_await std::get<ssl_socket_ptr>(socket_)->async_shutdown(boost::asio::use_awaitable);
}

void http_client_data_base::expires_after(std::chrono::seconds in_seconds) {
  timer_ptr_->expires_after(in_seconds);

  timer_ptr_->async_wait([this, _ = shared_from_this()](const boost::system::error_code& in_ec) {
    if (in_ec) {
      return;
    }
    do_close();
  });
}

http_client_data_base::socket_t& http_client_data_base::socket() {
  return std::visit(
    entt::overloaded{
        [](socket_ptr& in_socket) -> socket_t& { return *in_socket; },
        [](ssl_socket_ptr& in_socket) -> socket_t& { return in_socket->next_layer(); }
    },
    socket_
  );
}

http_client_data_base::ssl_socket_t* http_client_data_base::ssl_socket() {
  auto l_socket = std::get_if<ssl_socket_ptr>(&socket_);
  return l_socket ? l_socket->get() : nullptr;
}

void awaitable_queue::awaitable_queue_impl::await_suspend(std::function<void()> in_handle) {
  {
    const std::lock_guard l{lock_};
    next_list_.emplace(in_handle);
  }
}

bool awaitable_queue::awaitable_queue_impl::await_ready() { return !is_run_; }

void awaitable_queue::awaitable_queue_impl::next() {
  if (next_list_.empty()) return;
  const std::lock_guard l{lock_};
  next_list_.front()();
  next_list_.pop();
}

void awaitable_queue::awaitable_queue_impl::maybe_invoke() {
  if (is_run_) return;
  is_run_ = true;
  next();
}
} // namespace doodle::http::detail