//
// Created by TD on 24-7-15.
//
#include "doodle_core/core/global_function.h"
#include <doodle_core/core/app_base.h>
#include <doodle_core/core/doodle_lib.h>
#include <doodle_core/core/http_client_core.h>

#include <boost/asio/use_awaitable.hpp>
#include <boost/test/tools/interface.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/test/unit_test_log.hpp>

#include <utility>

namespace doodle::http {
template <typename SocketType>
class http_stream_base : public std::enable_shared_from_this<http_stream_base<SocketType>> {
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
  template <typename ResponseBody, typename RequestType, typename Handle>
  auto read_and_write(
      boost::beast::http::request<RequestType>&& in_req, boost::beast::http::response<ResponseBody>& out_res,
      Handle&& in_handle
  ) {
    in_req.payload_size();
    return boost::asio::async_compose<Handle, void(boost::system::error_code)>(
        read_and_write_compose<http_client, RequestType, ResponseBody>{
            std::move(in_req), this, boost::asio::coroutine{}, out_res
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
  template <typename ResponseBody, typename RequestType, typename Handle>
  auto read_and_write(
      boost::beast::http::request<RequestType>& in_req, boost::beast::http::response<ResponseBody>& out_res,
      Handle&& in_handle
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
void http_client_ssl::set_ssl() {
  socket_.set_verify_mode(boost::asio::ssl::verify_none);
  if (!SSL_set_tlsext_host_name(socket_.native_handle(), server_ip_.c_str()))
    throw_exception(doodle_error{"SSL_set_tlsext_host_name error"});
}
}  // namespace doodle::http

using namespace doodle;
BOOST_AUTO_TEST_SUITE(http_test)

class app_test : public app_base {
 public:
  app_test() : app_base() {}
  bool init() override {
    use_multithread(true);
    return true;
  }
};

BOOST_AUTO_TEST_CASE(quequ_t) {
  app_test l_app_base{};
  boost::asio::ssl::context l_ctx{boost::asio::ssl::context::tlsv12_client};
  auto l_c = std::make_shared<http::http_client_ssl>("https://www.baidu.com/", l_ctx);
  boost::beast::http::request<boost::beast::http::empty_body> l_req{boost::beast::http::verb::get, "/", 11};
  l_req.keep_alive(true);
  l_req.prepare_payload();
  boost::beast::http::response<boost::beast::http::string_body> l_res{};
  auto l_work                              = boost::asio::make_work_guard(g_io_context());
  std::shared_ptr<std::atomic_int32_t> l_p = std::make_shared<std::atomic_int32_t>(0);
  // for (int i = 0; i < 100; ++i) {
  l_c->read_and_write(l_req, l_res, [l_p, l_c, &l_res](boost::system::error_code in_ec) {
    if (in_ec) {
      BOOST_TEST_MESSAGE(in_ec.message());
      BOOST_TEST(false);
    } else {
      l_p->fetch_add(1);
      BOOST_TEST_MESSAGE(l_res.body());
      BOOST_TEST(true);
    }
  });
  // }

  l_app_base.run();
}

BOOST_AUTO_TEST_SUITE_END()