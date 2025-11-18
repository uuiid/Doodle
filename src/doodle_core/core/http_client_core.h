//
// Created by TD on 2024/3/6.
//

#pragma once
#include <doodle_core/configure/static_value.h>
#include <doodle_core/core/co_queue.h>
#include <doodle_core/core/core_set.h>
#include <doodle_core/core/global_function.h>
#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/lib_warp/boost_fmt_asio.h>
#include <doodle_core/lib_warp/boost_fmt_error.h>
#include <doodle_core/logger/logger.h>

#include <boost/algorithm/string.hpp>
#include <boost/asio.hpp>
#include <boost/asio/compose.hpp>
#include <boost/asio/experimental/channel.hpp>
#include <boost/asio/ts/netfwd.hpp>
#include <boost/beast.hpp>
#include <boost/beast/http/file_body_fwd.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/url.hpp>

#include <magic_enum/magic_enum_all.hpp>
#include <memory>
#include <string>
#include <utility>

namespace doodle::http {
template <typename SocketType>
class http_stream_base {
 protected:
  using resolver_t      = boost::asio::ip::tcp::resolver;
  using resolver_ptr    = std::shared_ptr<resolver_t>;

  using buffer_type     = boost::beast::flat_buffer;
  using channel_type    = boost::asio::experimental::channel<void()>;
  using socket_type     = SocketType;
  using socket_type_ptr = std::unique_ptr<socket_type>;

  template <typename SelfType, typename RequestType, typename ResponseBody>
  class read_and_write_compose_parser;

  buffer_type buffer_{};
  socket_type_ptr socket_;
  chrono::sys_time_pos last_use_time_;
  static constexpr auto default_timeout_ = chrono::seconds{30};

  chrono::seconds timeout_{default_timeout_};
  std::optional<chrono::seconds> next_timeout_{default_timeout_};

  virtual void expires_after_impl(std::chrono::seconds in_seconds) = 0;
  template <typename T>
  void set_request_timeout(boost::beast::http::request<T>& in_req) {
    if (timeout_ != default_timeout_ || (next_timeout_ && *next_timeout_ != default_timeout_)) {
      if (next_timeout_) {
        in_req.set(boost::beast::http::field::keep_alive, fmt::format("timeout={}", (*next_timeout_).count()));
      } else {
        in_req.set(boost::beast::http::field::keep_alive, fmt::format("timeout={}", timeout_.count()));
      }
    }
  }

 public:
  template <typename... Args>
  explicit http_stream_base(Args&&... args)
      : buffer_{},
        socket_(std::make_unique<socket_type>(std::forward<Args>(args)...)),
        last_use_time_(chrono::sys_time_pos::clock::now()) {}
  ~http_stream_base() = default;

  boost::urls::scheme scheme_id_{};
  std::string server_ip_{};
  std::string server_port_{};
  std::string server_ip_and_port_{};
  boost::asio::ip::tcp::resolver::results_type resolver_results_{};

  // 可选的 body 限制
  std::optional<std::size_t> body_limit_{};
  void expires_after(std::chrono::seconds in_seconds) {
    last_use_time_ = chrono::sys_time_pos::clock::now();
    if (next_timeout_) {
      timeout_ = *next_timeout_;
      next_timeout_.reset();
    }
    expires_after_impl(in_seconds);
  }

  void set_timeout(std::chrono::seconds in_seconds) { next_timeout_ = in_seconds; }
  const std::chrono::seconds& get_timeout() const { return timeout_; }

  // 解析url
  void parse_url(std::string in_url) {
    boost::urls::url l_url{in_url};
    server_ip_ = l_url.host();

    if (l_url.has_port())
      server_port_ = l_url.port();
    else if (l_url.scheme() == "https")
      server_port_ = "443";
    else
      server_port_ = "80";
    scheme_id_          = l_url.scheme_id();

    server_ip_and_port_ = server_ip_ + ":" + server_port_;
  };
  bool is_timeout() {
    auto l_now = chrono::sys_time_pos::clock::now();
    return (l_now - last_use_time_) > (timeout_ - 3s);
  }

  // copy constructor
  http_stream_base(const http_stream_base&)            = delete;
  // move constructor
  http_stream_base(http_stream_base&&)                 = delete;
  // copy assignment
  http_stream_base& operator=(const http_stream_base&) = delete;
  // move assignment
  http_stream_base& operator=(http_stream_base&&)      = delete;
};

class http_client : public http_stream_base<boost::beast::tcp_stream> {
  class resolve_and_connect_compose {
   public:
    http_client* self_;
    resolver_t resolver_;

    // 初次启动
    template <typename Self>
    void operator()(Self&& self) {
      resolver_.async_resolve(self_->server_ip_, self_->server_port_, std::move(self));
    }
    template <typename Self>
    void operator()(Self&& self, boost::system::error_code in_ec, resolver_t::results_type in_results) {
      self_->resolver_results_ = in_results;
      if (in_ec) {
        self.complete(in_ec);
        return;
      }
      self_->socket_->async_connect(self_->resolver_results_, std::move(self));
    }
    // 连接结束
    template <typename Self>
    void operator()(Self&& self, boost::system::error_code in_ec, boost::asio::ip::tcp::endpoint in_endpoint) {
      self.complete(in_ec);
    }
  };

 public:
  explicit http_client(std::string in_server_url) : http_stream_base(g_io_context()) { parse_url(in_server_url); }
  template <typename ExecutorType>
  explicit http_client(std::string in_server_url, ExecutorType&& in_executor)
      : http_stream_base(std::forward<ExecutorType>(in_executor)) {
    parse_url(in_server_url);
  }
  ~http_client() = default;

  // resolve and connect
  template <typename Handle>
  auto resolve_and_connect(Handle&& in_handle) {
    return boost::asio::async_compose<Handle, void(boost::system::error_code)>(
        resolve_and_connect_compose{this, resolver_t{socket_->get_executor()}}, in_handle
    );
  }
  bool is_open() { return socket_->socket().is_open(); }

  void reset_socket() { socket_ = std::make_unique<socket_type>(socket_->get_executor()); }
  void expires_after_impl(std::chrono::seconds in_seconds) { socket_->expires_after(in_seconds); }
  // read and write
  template <typename ResponseBody, typename RequestType, typename Handle>
  auto read_and_write(
      boost::beast::http::request<RequestType>& in_req, boost::beast::http::response<ResponseBody>& out_res,
      Handle&& in_handle
  ) {
    set_request_timeout(in_req);
    in_req.prepare_payload();
    return boost::asio::async_compose<Handle, void(boost::system::error_code)>(
        read_and_write_compose_parser<http_client, RequestType, ResponseBody>{
            in_req, this, boost::asio::coroutine{}, out_res
        },
        in_handle
    );
  }
};

class http_client_ssl : public http_stream_base<boost::beast::ssl_stream<boost::beast::tcp_stream>> {
  boost::asio::ssl::context& ctx_;
  void set_ssl();

  class resolve_and_connect_compose {
   public:
    http_client_ssl* self_;
    resolver_t resolver_;

    // 初次启动
    template <typename Self>
    void operator()(Self&& self) {
      resolver_.async_resolve(self_->server_ip_, self_->server_port_, std::move(self));
    }
    // 握手结束
    template <typename Self>
    void operator()(Self&& self, boost::system::error_code in_ec) {
      self.complete(in_ec);
    }

    template <typename Self>
    void operator()(Self&& self, boost::system::error_code in_ec, resolver_t::results_type in_results) {
      self_->resolver_results_ = in_results;
      if (in_ec) {
        self.complete(in_ec);
        return;
      }
      boost::beast::get_lowest_layer(*self_->socket_).async_connect(self_->resolver_results_, std::move(self));
    }
    template <typename Self>
    void operator()(Self&& self, boost::system::error_code in_ec, boost::asio::ip::tcp::endpoint in_endpoint) {
      if (in_ec) {
        self.complete(in_ec);
        return;
      }
      self_->socket_->async_handshake(boost::asio::ssl::stream_base::client, std::move(self));
    }
  };

 public:
  explicit http_client_ssl(std::string in_server_url, boost::asio::ssl::context& in_ctx)
      : http_stream_base(boost::beast::ssl_stream<boost::beast::tcp_stream>(g_io_context(), in_ctx)), ctx_(in_ctx) {
    parse_url(in_server_url);
    set_ssl();
  }

  template <typename ExecutorType>
  explicit http_client_ssl(std::string in_server_url, boost::asio::ssl::context& in_ctx, ExecutorType&& in_executor)
      : http_stream_base(
            boost::beast::ssl_stream<boost::beast::tcp_stream>(std::forward<ExecutorType>(in_executor), in_ctx)
        ),
        ctx_(in_ctx) {
    parse_url(in_server_url);
    set_ssl();
  }

  ~http_client_ssl() = default;

  // resolve and connect
  template <typename Handle>
  auto resolve_and_connect(Handle&& in_handle) {
    return boost::asio::async_compose<Handle, void(boost::system::error_code)>(
        resolve_and_connect_compose{this, resolver_t{socket_->get_executor()}}, in_handle
    );
  }

  bool is_open() { return socket_->next_layer().socket().is_open(); }
  void reset_socket() {
    socket_ = std::make_unique<boost::beast::ssl_stream<boost::beast::tcp_stream>>(socket_->get_executor(), ctx_);
    set_ssl();
  }
  void expires_after_impl(std::chrono::seconds in_seconds) {
    boost::beast::get_lowest_layer(*socket_).expires_after(in_seconds);
  }

  // read and write
  template <typename ResponseBody, typename RequestType, typename Handle = boost::asio::use_awaitable_t<>>
  auto read_and_write(
      boost::beast::http::request<RequestType>& in_req, boost::beast::http::response<ResponseBody>& out_res,
      Handle&& in_handle = boost::asio::use_awaitable
  ) {
    set_request_timeout(in_req);
    in_req.prepare_payload();
    return boost::asio::async_compose<Handle, void(boost::system::error_code)>(
        read_and_write_compose_parser<http_client_ssl, RequestType, ResponseBody>{
            in_req, this, boost::asio::coroutine{}, out_res
        },
        in_handle
    );
  }
};

template <typename SocketType>
template <typename SelfType, typename RequestType, typename ResponseBody>
class http_stream_base<SocketType>::read_and_write_compose_parser {
 public:
  boost::beast::http::request<RequestType>& req_;
  SelfType* self_;
  boost::asio::coroutine coro_;
  boost::beast::http::response<ResponseBody>& res_;
  std::shared_ptr<boost::beast::http::response_parser<ResponseBody>> parser_;

  explicit read_and_write_compose_parser(
      boost::beast::http::request<RequestType>& in_req, SelfType* in_self, boost::asio::coroutine in_coro,
      boost::beast::http::response<ResponseBody>& in_res
  )
      : req_(in_req),
        self_(in_self),
        coro_(in_coro),
        res_(in_res),
        parser_(std::make_shared<boost::beast::http::response_parser<ResponseBody>>(std::move(in_res))) {
    if (self_->body_limit_) parser_->body_limit(*self_->body_limit_);
  }

  template <typename Self>
  void operator()(Self&& self, boost::system::error_code in_ec = {}, std::size_t = 0) {
    BOOST_ASIO_CORO_REENTER(coro_) {
      // BOOST_ASIO_CORO_YIELD boost::beast::http::async_write(self_->socket_, std::as_const(req_), std::move(self));
      // if (in_ec) {
      if (self_->is_timeout()) {
        self_->reset_socket();
      }

      self_->expires_after(self_->timeout_);
      if (!self_->is_open()) {
        BOOST_ASIO_CORO_YIELD self_->resolve_and_connect(std::move(self));
        if (in_ec) goto end_complete;
      }

      BOOST_ASIO_CORO_YIELD boost::beast::http::async_write(*self_->socket_, req_, std::move(self));
      if (in_ec) goto end_complete;
      self_->expires_after(self_->timeout_);
      // }
      BOOST_ASIO_CORO_YIELD boost::beast::http::async_read(*self_->socket_, self_->buffer_, *parser_, std::move(self));
      if (in_ec) goto end_complete;
      self_->expires_after(self_->timeout_);
    end_complete:
      if (!in_ec) res_ = parser_->release();
      self.complete(in_ec);
    }
  }
};
}  // namespace doodle::http
