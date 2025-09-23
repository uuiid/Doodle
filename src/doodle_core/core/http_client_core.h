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
#include <utility>

namespace doodle::http::detail {

class http_client_data_base : public std::enable_shared_from_this<http_client_data_base> {
 public:
  using co_executor_type = boost::asio::as_tuple_t<boost::asio::use_awaitable_t<>>;
  using executor_type    = boost::asio::any_io_executor;

  using resolver_t       = co_executor_type::as_default_on_t<boost::asio::ip::tcp::resolver>;
  using resolver_ptr     = std::shared_ptr<resolver_t>;
  using buffer_type      = boost::beast::flat_buffer;

  using timer_t          = boost::asio::steady_timer;
  using timer_ptr_t      = std::shared_ptr<timer_t>;

  using socket_t         = co_executor_type::as_default_on_t<boost::beast::tcp_stream>;
  using socket_ptr       = std::shared_ptr<socket_t>;

  using ssl_socket_t     = boost::beast::ssl_stream<socket_t>;
  using ssl_socket_ptr   = std::shared_ptr<ssl_socket_t>;

 private:
  boost::asio::any_io_executor executor_;
  boost::asio::ssl::context* ctx_{};
  boost::urls::scheme scheme_id_{};
  template <typename ResponseBody, typename RequestType>
  friend boost::asio::awaitable<std::tuple<boost::system::error_code, boost::beast::http::response<ResponseBody>>>
  read_and_write(
      std::shared_ptr<http_client_data_base> in_client_data, boost::beast::http::request<RequestType> in_req
  );
  bool is_open_and_cond_{false};

 public:
  http_client_data_base()  = default;
  ~http_client_data_base() = default;

  template <typename ExecutorType>
  explicit http_client_data_base(ExecutorType&& in_executor) : executor_(boost::asio::make_strand(in_executor)) {}

  boost::asio::any_io_executor get_executor() const { return executor_; }

  std::string server_ip_{};
  std::string server_port_{};
  logger_ptr logger_{};
  resolver_ptr resolver_{};
  boost::asio::ip::tcp::resolver::results_type resolver_results_;
  buffer_type buffer_{};
  std::variant<socket_ptr, ssl_socket_ptr> socket_{};

  // 定时关闭
  timer_ptr_t timer_ptr_;
  boost::system::error_code error_{};

  // 异步队列
  awaitable_queue_limitation queue_;

  virtual void init(std::string in_server_url, boost::asio::ssl::context* in_ctx = nullptr);

  void expires_after(std::chrono::seconds in_seconds);
  void do_close();

  // 在测试打开为真时,一定会返回
  socket_t& socket();
  // 在测试打开为真时, 有可能返回空指针
  ssl_socket_t* ssl_socket();
  // 重新初始化
  void re_init();
  bool is_open();

  // 获取 set-cookie 中的 key
  template <typename Resp>
  static auto get_header_resp(Resp& in_resp, const std::string& in_key) {
    auto l_set_cookie = in_resp[boost::beast::http::field::set_cookie];
    if (!l_set_cookie.empty()) {
      std::vector<std::string> l_cookie;
      boost::split(l_cookie, l_set_cookie, boost::is_any_of(";"));
      for (auto& l_item : l_cookie) {
        if (l_item.starts_with(in_key)) {
          return l_item.substr(in_key.size() + 1);  // +1 是等号
        }
      }
    }
    return std::string{};
  }

  // 替换或者添加 cookie
  template <typename Resp>
  static void set_header_resp(Resp& in_resp, const std::string& in_key, const std::string& in_value) {
    auto l_cookie = in_resp[boost::beast::http::field::cookie];
    if (!l_cookie.empty()) {
      std::vector<std::string> l_cookies;
      boost::split(l_cookies, l_cookie, boost::is_any_of(";"));
      for (auto& l_item : l_cookies) {
        if (l_item.starts_with(in_key)) {
          l_item = in_key + "=" + in_value;
        }
      }
      in_resp.set(boost::beast::http::field::cookie, boost::algorithm::join(l_cookies, ";\n"));
    } else
      in_resp.set(boost::beast::http::field::cookie, in_key + "=" + in_value);
  }
};
using http_client_data_base_ptr = std::shared_ptr<http_client_data_base>;
template <typename ResponseBody, typename RequestType>
boost::asio::awaitable<std::tuple<boost::system::error_code, boost::beast::http::response<ResponseBody>>>
read_and_write(std::shared_ptr<http_client_data_base> in_client_data, boost::beast::http::request<RequestType> in_req) {
  in_req.payload_size();
  using buffer_type = boost::beast::flat_buffer;
  auto l_g          = co_await in_client_data->queue_.queue(boost::asio::use_awaitable);
  auto l_work_guard = boost::asio::make_work_guard(in_client_data->get_executor());
  in_client_data->expires_after(30s);

  boost::beast::http::response<ResponseBody> l_ret{};
  if (!in_client_data->is_open()) {
    in_client_data->re_init();
    std::tie(in_client_data->error_, in_client_data->resolver_results_) =
        co_await in_client_data->resolver_->async_resolve(in_client_data->server_ip_, in_client_data->server_port_);
    if (in_client_data->error_) {
      in_client_data->logger_->log(log_loc(), level::err, "async_resolve error: {}", in_client_data->error_.message());
      co_return std::make_tuple(in_client_data->error_, l_ret);
    }
    in_client_data->expires_after(std::chrono::seconds{10});

    std::tie(in_client_data->error_, std::ignore) =
        co_await in_client_data->socket().async_connect(in_client_data->resolver_results_);
    if (in_client_data->error_) {
      in_client_data->logger_->log(log_loc(), level::err, "async_connect error: {}", in_client_data->error_.message());
      co_return std::make_tuple(in_client_data->error_, l_ret);
    }

    if (auto l_ssl = in_client_data->ssl_socket(); l_ssl) {
      std::tie(in_client_data->error_) = co_await l_ssl->async_handshake(boost::asio::ssl::stream_base::client);
      if (in_client_data->error_) {
        in_client_data->logger_->log(
            log_loc(), level::err, "async_handshake error: {}", in_client_data->error_.message()
        );
        co_return std::make_tuple(in_client_data->error_, l_ret);
      }
    }
    in_client_data->is_open_and_cond_ = true;
  }
  using visit_return_type = boost::asio::async_result<
      http_client_data_base::co_executor_type, void(boost::system::error_code, std::size_t)>::return_type;

  in_client_data->expires_after(30s);
  std::size_t l_size{};
  std::tie(in_client_data->error_, l_size) = co_await std::visit(
      [in_req_ptr = &in_req](auto&& in_socket_ptr) -> visit_return_type {
        // 此处调整异步堆栈
        auto l_req = in_req_ptr;
        co_return co_await boost::beast::http::async_write(*in_socket_ptr, *l_req);
      },
      in_client_data->socket_
  );

  if (in_client_data->error_) {
    in_client_data->logger_->log(log_loc(), level::err, "async_write error: {}", in_client_data->error_.message());
    co_return std::make_tuple(in_client_data->error_, l_ret);
  }
  in_client_data->expires_after(30s);

  std::tie(in_client_data->error_, l_size) = co_await std::visit(
      [in_ret_ptr = &l_ret](auto&& in_socket_ptr) -> visit_return_type {
        // 此处调整异步堆栈
        auto l_ret_ptr = in_ret_ptr;
        buffer_type l_buffer2{};
        co_return co_await boost::beast::http::async_read(*in_socket_ptr, l_buffer2, *l_ret_ptr);
      },
      in_client_data->socket_
  );
  if (in_client_data->error_) {
    in_client_data->logger_->log(log_loc(), level::err, "async_read error: {}", in_client_data->error_.message());
    co_return std::make_tuple(in_client_data->error_, l_ret);
  }

  co_return std::make_tuple(boost::system::error_code{}, l_ret);
}

}  // namespace doodle::http::detail

namespace doodle::http {
template <typename SocketType>
class http_stream_base {
 protected:
  using resolver_t   = boost::asio::ip::tcp::resolver;
  using resolver_ptr = std::shared_ptr<resolver_t>;

  using buffer_type  = boost::beast::flat_buffer;
  using channel_type = boost::asio::experimental::channel<void()>;
  using socket_type  = SocketType;

  template <typename SelfType, typename RequestType, typename ResponseBody>
  class read_and_write_compose;

 public:
  template <typename... Args>
  explicit http_stream_base(Args&&... args) : buffer_{}, socket_(std::forward<Args>(args)...) {}
  ~http_stream_base() = default;

  boost::urls::scheme scheme_id_{};
  std::string server_ip_{};
  std::string server_port_{};
  boost::asio::ip::tcp::resolver::results_type resolver_results_{};
  buffer_type buffer_{};

  socket_type socket_;
  // channel_type queue_;

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
    scheme_id_ = l_url.scheme_id();
  };

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
      self_->socket_.async_connect(self_->resolver_results_, std::move(self));
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
        resolve_and_connect_compose{this, resolver_t{socket_.get_executor()}}, in_handle
    );
  }
  bool is_open() { return socket_.socket().is_open(); }
  void expires_after(std::chrono::seconds in_seconds) { socket_.expires_after(in_seconds); }
  // read and write
  template <typename ResponseBody, typename RequestType, typename Handle = boost::asio::use_awaitable_t<>>
  auto read_and_write(
      boost::beast::http::request<RequestType>& in_req, boost::beast::http::response<ResponseBody>& out_res,
      Handle&& in_handle = boost::asio::use_awaitable
  ) {
    in_req.payload_size();
    return boost::asio::async_compose<Handle, void(boost::system::error_code)>(
        read_and_write_compose<http_client, RequestType, ResponseBody>{in_req, this, boost::asio::coroutine{}, out_res},
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
      boost::beast::get_lowest_layer(self_->socket_).async_connect(self_->resolver_results_, std::move(self));
    }
    template <typename Self>
    void operator()(Self&& self, boost::system::error_code in_ec, boost::asio::ip::tcp::endpoint in_endpoint) {
      if (in_ec) {
        self.complete(in_ec);
        return;
      }
      self_->socket_.async_handshake(boost::asio::ssl::stream_base::client, std::move(self));
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
        resolve_and_connect_compose{this, resolver_t{socket_.get_executor()}}, in_handle
    );
  }

  bool is_open() { return socket_.next_layer().socket().is_open(); }
  void expires_after(std::chrono::seconds in_seconds) {
    boost::beast::get_lowest_layer(socket_).expires_after(in_seconds);
  }

  // read and write
  template <typename ResponseBody, typename RequestType, typename Handle = boost::asio::use_awaitable_t<>>
  auto read_and_write(
      boost::beast::http::request<RequestType>& in_req, boost::beast::http::response<ResponseBody>& out_res,
      Handle&& in_handle = boost::asio::use_awaitable
  ) {
    in_req.payload_size();
    return boost::asio::async_compose<Handle, void(boost::system::error_code)>(
        read_and_write_compose<http_client_ssl, RequestType, ResponseBody>{
            in_req, this, boost::asio::coroutine{}, out_res
        },
        in_handle
    );
  }
};

template <typename SocketType>
template <typename SelfType, typename RequestType, typename ResponseBody>
class http_stream_base<SocketType>::read_and_write_compose {
 public:
  boost::beast::http::request<RequestType>& req_;
  SelfType* self_;
  boost::asio::coroutine coro_;
  boost::beast::http::response<ResponseBody>& res_;

  template <typename Self>
  void operator()(Self&& self, boost::system::error_code in_ec = {}, std::size_t = 0) {
    BOOST_ASIO_CORO_REENTER(coro_) {
      // BOOST_ASIO_CORO_YIELD boost::beast::http::async_write(self_->socket_, std::as_const(req_), std::move(self));
      // if (in_ec) {
      if (!self_->is_open()) {
        BOOST_ASIO_CORO_YIELD self_->resolve_and_connect(std::move(self));
        if (in_ec) goto end_complete;
      }

      BOOST_ASIO_CORO_YIELD boost::beast::http::async_write(self_->socket_, std::as_const(req_), std::move(self));
      if (in_ec) goto end_complete;
      self_->expires_after(30s);
      // }
      BOOST_ASIO_CORO_YIELD boost::beast::http::async_read(self_->socket_, self_->buffer_, res_, std::move(self));
      if (in_ec) goto end_complete;
      self_->expires_after(30s);
    end_complete:
      self.complete(in_ec);
    }
  }
};

}  // namespace doodle::http
