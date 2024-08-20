//
// Created by TD on 2024/3/6.
//

#include "http_client_core.h"

#include "app_base.h"

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
  scheme_id_ = l_url.scheme_id();
  ctx_       = in_ctx;

  switch (scheme_id_) {
    case boost::urls::scheme::http:  // http
      socket_ = std::make_shared<socket_t>(executor_);
      break;
    case boost::urls::scheme::https:  // https
    {
      if (ctx_ == nullptr) {
        logger_->log(log_loc(), level::err, "https 需要 ssl context");
        return;
      }
      auto l_ssl = std::make_shared<ssl_socket_t>(executor_, *ctx_);
      l_ssl->set_verify_mode(boost::asio::ssl::verify_none);
      if (!SSL_set_tlsext_host_name(l_ssl->native_handle(), server_ip_.c_str())) {
        boost::system::error_code ec{static_cast<int>(::ERR_get_error()), boost::asio::error::get_ssl_category()};
        logger_->log(log_loc(), level::err, "SSL_set_tlsext_host_name error: {}", ec.message());
        return;
      }
      socket_ = l_ssl;
    } break;
    default:
      logger_->log(log_loc(), level::err, "不支持的协议 {}", l_url.scheme());
      break;
  }
}

void http_client_data_base::re_init() {
  if (is_open()) return;
  if (std::visit([](auto&& in_socket_ptr) -> bool { return !!in_socket_ptr; }, socket_) && socket().socket().is_open())
    return;

  switch (scheme_id_) {
    case boost::urls::scheme::http:  // http
      socket_ = std::make_shared<socket_t>(executor_);
      break;
    case boost::urls::scheme::https:  // https
    {
      if (ctx_ == nullptr) {
        logger_->log(log_loc(), level::err, "https 需要 ssl context");
        return;
      }
      auto l_ssl = std::make_shared<ssl_socket_t>(executor_, *ctx_);
      l_ssl->set_verify_mode(boost::asio::ssl::verify_none);
      if (!SSL_set_tlsext_host_name(l_ssl->native_handle(), server_ip_.c_str())) {
        boost::system::error_code ec{static_cast<int>(::ERR_get_error()), boost::asio::error::get_ssl_category()};
        logger_->log(log_loc(), level::err, "SSL_set_tlsext_host_name error: {}", ec.message());
        return;
      }
      socket_ = l_ssl;
    } break;
    default:
      logger_->log(log_loc(), level::err, "不支持的协议 {}", boost::urls::to_string(scheme_id_));
      break;
  }
}

bool http_client_data_base::is_open() {
  return std::visit([](auto&& in_socket_ptr) -> bool { return !!in_socket_ptr; }, socket_) &&
         socket().socket().is_open() && is_open_and_cond_;
}

void http_client_data_base::do_close() {
  is_open_and_cond_ = false;
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
  boost::system::error_code ec;
  timer_ptr_->cancel(ec);
  if (ec) {
    logger_->log(log_loc(), level::err, "do_close error: {}", ec.message());
  }
  // co_await std::get<ssl_socket_ptr>(socket_)->async_shutdown(boost::asio::use_awaitable);
}

void http_client_data_base::expires_after(std::chrono::seconds in_seconds) {
  timer_ptr_->expires_after(in_seconds);
  std::visit(
      entt::overloaded{
          [&](socket_ptr& in_socket) {
            if (in_socket) in_socket->expires_after(in_seconds);
          },
          [&](ssl_socket_ptr& in_socket) {
            if (in_socket) boost::beast::get_lowest_layer(*in_socket).expires_after(in_seconds);
          }
      },
      socket_
  );

  timer_ptr_->async_wait(boost::asio::bind_cancellation_slot(
      app_base::Get().on_cancel.slot(),
      [this, _ = shared_from_this()](const boost::system::error_code& in_ec) {
        if (in_ec) {
          return;
        }
        do_close();
      }
  ));
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

}  // namespace doodle::http::detail