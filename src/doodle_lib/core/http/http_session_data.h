//
// Created by TD on 2024/2/20.
//

#pragma once

#include <doodle_core/core/wait_op.h>
#include <doodle_core/lib_warp/boost_fmt_error.h>

#include <doodle_lib/core/http/http_websocket_data.h>
#include <doodle_lib/core/http/socket_logger.h>

#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/url.hpp>

namespace doodle::http {
struct capture_t;
class http_route;
using http_route_ptr = std::shared_ptr<http_route>;
class http_session_data;
using http_session_data_ptr = std::shared_ptr<http_session_data>;

class http_session_data : public std::enable_shared_from_this<http_session_data> {
 private:
  void do_read(boost::system::error_code ec, std::size_t bytes_transferred);
  void do_send(boost::system::error_code ec, std::size_t bytes_transferred);
  std::uint32_t version_{};
  bool keep_alive_{};

 public:
  explicit http_session_data(boost::asio::ip::tcp::socket in_socket, http_route_ptr in_route_ptr)
      : stream_(std::make_unique<boost::beast::tcp_stream>(std::move(in_socket))),
        buffer_{},
        url_{},
        request_parser_{},
        route_ptr_{std::move(in_route_ptr)} {
    logger_ = g_logger_ctrl().make_log(fmt::format("{}_{}", "socket", SOCKET(stream_->socket().native_handle())));
  }
  ~http_session_data() = default;
  std::unique_ptr<boost::beast::tcp_stream> stream_;
  boost::beast::flat_buffer buffer_;
  boost::url url_;
  std::unique_ptr<boost::beast::http::request_parser<boost::beast::http::empty_body>> request_parser_;
  logger_ptr logger_{};
  std::shared_ptr<capture_t> capture_{};
  http_route_ptr route_ptr_{};

  std::shared_ptr<void> request_body_parser_;

  inline boost::beast::tcp_stream& operator*() const { return *stream_; }
  inline boost::beast::tcp_stream* operator->() const { return stream_.get(); }

  // get msg body parser
  template <typename MsgBody>
  auto get_msg_body_parser() const {
    return std::static_pointer_cast<session::async_read_body<MsgBody>>(request_body_parser_);
  }

  // copy delete
  http_session_data(const http_session_data&)                = delete;
  http_session_data& operator=(const http_session_data&)     = delete;
  // move
  http_session_data(http_session_data&&) noexcept            = default;
  http_session_data& operator=(http_session_data&&) noexcept = default;

  void rend_head();

  void do_close();
  void seed_error(boost::beast::http::status in_status, boost::system::error_code ec, const std::string& in_str = "");
  void seed(boost::beast::http::message_generator in_message_generator);
};

namespace session {

template <typename Handler>
struct http_method_base : doodle::detail::wait_op {
 public:
  http_session_data_ptr handle_{};

  explicit http_method_base(Handler&& handler)
      : doodle::detail::wait_op(&http_method_base::on_complete, std::make_shared<Handler>(std::move(handler))){};
  ~http_method_base() = default;

 private:
  static void on_complete(wait_op* op) {
    auto l_self = static_cast<http_method_base*>(op);
    boost::asio::post(
        boost::asio::prepend(std::move(*static_cast<Handler*>(l_self->handler_.get())), l_self->ec_, l_self->handle_)
    );
  }
};

template <typename MsgBody>
struct async_read_body;

template <typename MsgBody>
struct async_read_body {
 public:
  using set_handle_fun_t = void (*)(std::shared_ptr<doodle::detail::wait_op>, http_session_data_ptr);

 private:
  set_handle_fun_t set_handle_fun_{};

 public:
  using request_parser_t = boost::beast::http::request_parser<MsgBody>;
  std::unique_ptr<request_parser_t> request_parser_;
  std::shared_ptr<doodle::detail::wait_op> wait_op_{};
  http_session_data& handle_;

  explicit async_read_body(const http_session_data_ptr& in_handle)
      : request_parser_(
            std::make_unique<boost::beast::http::request_parser<MsgBody>>(std::move(in_handle->request_parser_))
        ),
        handle_(*in_handle){};

  // copy delete
  async_read_body(const async_read_body&)                = delete;
  async_read_body& operator=(const async_read_body&)     = delete;
  // move
  async_read_body(async_read_body&&) noexcept            = default;
  async_read_body& operator=(async_read_body&&) noexcept = default;
  template <typename CompletionHandler>
  auto async_end(CompletionHandler&& in_handler) {
    return boost::asio::async_initiate<CompletionHandler, void(boost::system::error_code, http_session_data_ptr)>(
        [this](auto&& handler) {
          using http_method_base_t = http_method_base<std::decay_t<decltype(handler)>>;
          auto l_op                = std::make_shared<http_method_base_t>(std::forward<decltype(handler)>(handler));
          wait_op_                 = l_op;
          set_handle_fun_ = [](std::shared_ptr<doodle::detail::wait_op> in_wait_op, http_session_data_ptr in_handle) {
            std::static_pointer_cast<http_method_base_t>(in_wait_op)->handle_ = std::move(in_handle);
          };
          rend_body();
        },
        in_handler
    );
  }

  void rend_body() {
    handle_.stream_->expires_after(30s);
    boost::beast::http::async_read(
        *handle_.stream_, handle_.buffer_, *request_parser_,
        boost::beast::bind_front_handler(&async_read_body::do_read, this)
    );
  }
  void do_read(boost::system::error_code ec, std::size_t bytes_transferred) {
    set_handle_fun_(wait_op_, handle_.shared_from_this());
    if (ec) {
      handle_.logger_->log(log_loc(), level::err, fmt::format("读取头部失败 {}", ec.message()));
      wait_op_->ec_ = ec;
    }
    wait_op_->complete();
  }
};

template <typename MsgBody, typename CompletionHandler>
auto make_http_reg_fun(CompletionHandler&& in_handler) {
  return [in_handler = std::forward<CompletionHandler>(in_handler)](const http_session_data_ptr& in_handle) mutable {
    auto l_read = std::make_shared<async_read_body<MsgBody>>(in_handle);
    l_read->async_end(std::move(boost::asio::consign(in_handler, in_handle)));
    in_handle->request_body_parser_ = l_read;
  };
}
template <typename CompletionHandler>
auto make_http_reg_fun(CompletionHandler&& in_handler) {
  return [in_handler = std::forward<CompletionHandler>(in_handler)](const http_session_data_ptr& in_handle) {
    boost::asio::post(boost::asio::prepend(in_handler, boost::system::error_code{}, in_handle));
  };
}

template <typename CompletionHandler, typename CompletionHandlerWebSocket>
auto make_http_reg_fun(CompletionHandler&& in_handler1, CompletionHandlerWebSocket&& in_handler2) {
  return [in_handler1 = std::forward<CompletionHandler>(in_handler1),
          in_handler2 = std::forward<CompletionHandlerWebSocket>(in_handler2)](const http_session_data_ptr& in_handle) {
    if (boost::beast::websocket::is_upgrade(in_handle->request_parser_->get())) {
      boost::beast::get_lowest_layer(*in_handle->stream_).expires_never();
      auto l_web_socket = std::make_shared<http_websocket_data>(std::move(*in_handle->stream_));
      l_web_socket->run(in_handle);
      boost::asio::post(boost::asio::prepend(in_handler2, boost::system::error_code{}, l_web_socket));
    } else {
      boost::asio::post(boost::asio::prepend(in_handler1, boost::system::error_code{}, in_handle));
    }
  };
}
}  // namespace session

}  // namespace doodle::http