//
// Created by TD on 2024/2/20.
//

#pragma once

#include <doodle_core/core/wait_op.h>

#include <doodle_lib/core/http/socket_logger.h>

#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/url.hpp>

namespace doodle::http {
class http_session_data {
 private:
  void do_read(boost::system::error_code ec, std::size_t bytes_transferred);
  void do_send(boost::system::error_code ec, std::size_t bytes_transferred);
  std::uint32_t version_{};
  bool keep_alive_{};

 public:
  explicit http_session_data(boost::asio::ip::tcp::socket in_socket)
      : stream_(std::make_unique<boost::beast::tcp_stream>(std::move(in_socket))),
        buffer_{},
        url_{},
        request_parser_{} {}
  std::unique_ptr<boost::beast::tcp_stream> stream_;
  boost::beast::flat_buffer buffer_;
  boost::url url_;
  std::unique_ptr<boost::beast::http::request_parser<boost::beast::http::empty_body>> request_parser_;

  inline boost::beast::tcp_stream& operator*() const { return *stream_; }
  inline boost::beast::tcp_stream* operator->() const { return stream_.get(); }

  // copy delete
  http_session_data(const http_session_data&)                = delete;
  http_session_data& operator=(const http_session_data&)     = delete;
  // move
  http_session_data(http_session_data&&) noexcept            = default;
  http_session_data& operator=(http_session_data&&) noexcept = default;

  void rend_head();

  void do_close();
  void seed_error(boost::beast::http::status in_status, boost::system::error_code ec);
  void seed(boost::beast::http::message_generator in_message_generator);
};

namespace session {

template <typename Handler>
struct http_method_base : doodle::detail::wait_op {
 public:
  entt::handle handle_{};

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
  using set_handle_fun_t = void (*)(std::shared_ptr<doodle::detail::wait_op>, entt::handle);

 private:
  set_handle_fun_t set_handle_fun_{};

 public:
  using request_parser_t = boost::beast::http::request_parser<MsgBody>;
  std::unique_ptr<request_parser_t> request_parser_;
  std::shared_ptr<doodle::detail::wait_op> wait_op_{};

  explicit async_read_body(const entt::handle& in_handle)
      : request_parser_(std::make_unique<boost::beast::http::request_parser<MsgBody>>(
            std::move(*in_handle.get<http_session_data>().request_parser_)
        )){};

  // copy delete
  async_read_body(const async_read_body&)                = delete;
  async_read_body& operator=(const async_read_body&)     = delete;
  // move
  async_read_body(async_read_body&&) noexcept            = default;
  async_read_body& operator=(async_read_body&&) noexcept = default;
  template <typename CompletionHandler>
  auto async_end(CompletionHandler&& in_handler) {
    return boost::asio::async_initiate<CompletionHandler, void(boost::system::error_code, entt::handle)>(
        [this](auto&& handler) {
          using http_method_base_t = http_method_base<std::decay_t<decltype(handler)>>;
          auto l_op                = std::make_shared<http_method_base_t>(std::forward<decltype(handler)>(handler));
          wait_op_                 = l_op;
          set_handle_fun_          = [](std::shared_ptr<doodle::detail::wait_op> in_wait_op, entt::handle in_handle) {
            std::static_pointer_cast<http_method_base_t>(in_wait_op)->handle_ = std::move(in_handle);
          };
          rend_body();
        },
        in_handler
    );
  }

  void rend_body() {
    auto l_self_handle = entt::handle{*g_reg(), entt::to_entity(*g_reg(), *this)};
    auto& l_data       = l_self_handle.get<http_session_data>();
    l_data.stream_->expires_after(30s);
    boost::beast::http::async_read(
        *l_data.stream_, l_data.buffer_, *request_parser_,
        boost::beast::bind_front_handler(&async_read_body::do_read, this)
    );
  }
  void do_read(boost::system::error_code ec, std::size_t bytes_transferred) {
    auto l_self_handle = entt::handle{*g_reg(), entt::to_entity(*g_reg(), *this)};
    auto l_logger      = l_self_handle.get<socket_logger>().logger_;
    auto& l_data       = l_self_handle.get<http_session_data>();
    set_handle_fun_(wait_op_, l_self_handle);
    if (ec) {
      l_logger->log(log_loc(), level::err, fmt::format("读取头部失败 {}", ec.message()));
      wait_op_->ec_ = ec;
    }
    wait_op_->complete();
  }
};

class http_method_web_socket {
 public:
  template <typename Handler>
  struct http_method_base_lasting : doodle::detail::wait_op_lasting {
   public:
    entt::handle handle_{};

    explicit http_method_base_lasting(Handler&& handler)
        : doodle::detail::wait_op_lasting(
              &http_method_base_lasting::on_complete, std::make_shared<Handler>(std::move(handler))
          ){};
    ~http_method_base_lasting() = default;

   private:
    static void on_complete(wait_op_lasting* op) {
      auto l_self = static_cast<http_method_base_lasting*>(op);
      boost::asio::post(
          boost::asio::prepend(std::move(*static_cast<Handler*>(l_self->handler_.get())), l_self->ec_, l_self->handle_)
      );
    }
  };

  using set_handle_fun_t = void (*)(std::shared_ptr<doodle::detail::wait_op_lasting>, entt::handle);

 private:
  set_handle_fun_t set_handle_fun_{};
  std::shared_ptr<doodle::detail::wait_op_lasting> wait_op_{};

 protected:
  void upgrade_websocket(const entt::handle& in_handle) const;

 public:
  template <typename CompletionHandler>
  explicit http_method_web_socket(CompletionHandler&& in_handler) {
    async_end(std::forward<CompletionHandler>(in_handler));
  }

  template <typename CompletionHandler>
  auto async_end(CompletionHandler&& in_handler) {
    return boost::asio::async_initiate<CompletionHandler, void(boost::system::error_code, entt::handle)>(
        [this](auto&& handler) {
          using http_method_base_t = http_method_base_lasting<std::decay_t<decltype(handler)>>;
          auto l_op                = std::make_shared<http_method_base_t>(std::forward<decltype(handler)>(handler));
          wait_op_                 = l_op;
          set_handle_fun_ = [](std::shared_ptr<doodle::detail::wait_op_lasting> in_wait_op, entt::handle in_handle) {
            std::static_pointer_cast<http_method_base_t>(in_wait_op)->handle_ = std::move(in_handle);
          };
        },
        in_handler
    );
  }
  void operator()(const entt::handle& in_handle) const;
};

template <typename MsgBody, typename CompletionHandler>
auto make_http_reg_fun(CompletionHandler&& in_handler) {
  return [in_handler = std::forward<CompletionHandler>(in_handler)](const entt::handle& in_handle) {
    auto& l_read = in_handle.emplace_or_replace<async_read_body<MsgBody>>(in_handle);
    l_read.async_end(in_handler);
  };
}
template <bool has_websocket, typename CompletionHandler>
auto make_http_reg_fun(CompletionHandler&& in_handler) {
  if constexpr (has_websocket) {
    return http_method_web_socket{std::forward<CompletionHandler>(in_handler)};
  } else {
    return [in_handler = std::forward<CompletionHandler>(in_handler)](const entt::handle& in_handle) {
      boost::asio::post(boost::asio::prepend(std::move(in_handler), boost::system::error_code{}, in_handle));
    };
  }
}
}  // namespace session

}  // namespace doodle::http