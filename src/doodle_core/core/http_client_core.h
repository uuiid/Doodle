//
// Created by TD on 2024/3/6.
//

#pragma once
#include <doodle_core/configure/static_value.h>
#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/lib_warp/boost_fmt_asio.h>
#include <doodle_core/lib_warp/boost_fmt_error.h>
#include <doodle_core/logger/logger.h>

#include <boost/asio.hpp>
#include <boost/asio/ts/netfwd.hpp>
#include <boost/beast.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/url.hpp>

#include <magic_enum.hpp>
namespace doodle::http::detail {

template <typename, typename, bool>
class http_client_core;

namespace http_client_core_ns {
enum state {
  start,
  resolve,
  connect,
  write,
  read,
};
inline auto format_as(state f) { return magic_enum::enum_name(f); }
}  // namespace http_client_core_ns
class response_header_operator_base {
 public:
  response_header_operator_base() {}
  ~response_header_operator_base() = default;
  template <typename T, typename ResponeType>
  void operator()(T* in_http_client_core, ResponeType& in_req) {}
};

class request_header_operator_base {
 public:
  request_header_operator_base() {}
  ~request_header_operator_base() = default;
  template <typename T, typename RequestType>
  void operator()(T* in_http_client_core, RequestType& in_req) {
    in_req.set(
        boost::beast::http::field::host,
        fmt::format("{}:{}", in_http_client_core->server_ip(), in_http_client_core->server_port())
    );
    in_req.set(boost::beast::http::field::user_agent, BOOST_BEAST_VERSION_STRING);
  }
};

template <typename RequestOperator, typename ResponseOperator, bool use_ssl = false>
class http_client_core
    : public std::enable_shared_from_this<http_client_core<RequestOperator, ResponseOperator, use_ssl>> {
 public:
  using socket_t                      = boost::beast::tcp_stream;
  using socket_ptr                    = std::shared_ptr<socket_t>;
  using ssl_socket_t                  = boost::beast::ssl_stream<boost::beast::tcp_stream>;
  using ssl_socket_ptr                = std::shared_ptr<ssl_socket_t>;

  using resolver_t                    = boost::asio::ip::tcp::resolver;
  using resolver_ptr                  = std::shared_ptr<resolver_t>;
  using buffer_type                   = boost::beast::flat_buffer;
  using response_header_operator_type = ResponseOperator;
  using request_header_operator_type  = RequestOperator;

 private:
  using next_fun_t          = std::function<void()>;
  using next_fun_ptr_t      = std::shared_ptr<next_fun_t>;
  using next_fun_weak_ptr_t = std::weak_ptr<next_fun_t>;
  template <bool use_ssl>
  struct data_type;

  template <>
  struct data_type<false> {
    std::string server_ip_;
    std::string server_port_;
    socket_ptr socket_{};
    logger_ptr logger_{};
    resolver_ptr resolver_{};
    std::string scheme_{};  // http or https
    boost::asio::ip::tcp::resolver::results_type resolver_results_;

    std::queue<boost::beast::saved_handler> next_list_;
  };

  template <>
  struct data_type<true> {
    explicit data_type(boost::asio::ssl::context& in_ctx) : ssl_ctx_(in_ctx) {}
    boost::asio::ssl::context& ssl_ctx_;
    std::string server_ip_;
    std::string server_port_;
    ssl_socket_ptr socket_{};
    logger_ptr logger_{};
    resolver_ptr resolver_{};
    std::string scheme_{};  // http or https
    boost::asio::ip::tcp::resolver::results_type resolver_results_;

    std::queue<boost::beast::saved_handler> next_list_;
  };

  std::shared_ptr<data_type<use_ssl>> ptr_;

  using timer_t     = boost::asio::steady_timer;
  using timer_ptr_t = std::shared_ptr<timer_t>;
  timer_ptr_t timer_ptr_;

  bool is_run_ = false;
  request_header_operator_type request_header_operator_;
  response_header_operator_type response_header_operator_;

  // is_run 守卫
  struct guard_is_run {
    http_client_core& http_client_core_;
    explicit guard_is_run(http_client_core& in_core) : http_client_core_(in_core) { http_client_core_.is_run_ = true; }
    ~guard_is_run() {
      http_client_core_.is_run_ = false;
      http_client_core_.next();
    }
  };

 public:
  using state = http_client_core_ns::state;

 private:
  template <typename ExecutorType, typename CompletionHandler, typename ResponseType, typename RequestType>
  struct connect_write_read_op : boost::beast::async_base<std::decay_t<CompletionHandler>, ExecutorType>,
                                 boost::asio::coroutine {
    struct data_type2 {
      buffer_type buffer_;
      ResponseType response_;
      state state_ = state::start;
      RequestType request_;

      logger_ptr logger_;
      // 重试次数
      std::size_t retry_count_{};
    };
    boost::asio::executor_work_guard<ExecutorType> work_guard_;
    std::unique_ptr<data_type2> ptr_;
    http_client_core* http_client_core_ptr_;
    std::shared_ptr<guard_is_run> guard_is_run_ptr_;

    using base_type = boost::beast::async_base<std::decay_t<CompletionHandler>, ExecutorType>;
    explicit connect_write_read_op(
        http_client_core* in_ptr, RequestType&& in_req, CompletionHandler&& in_handler,
        const ExecutorType& in_executor_type_1
    )
        : base_type(std::move(in_handler), in_executor_type_1),
          boost::asio::coroutine(),
          work_guard_(boost::asio::make_work_guard(in_executor_type_1)),
          ptr_(std::make_unique<data_type2>()),
          http_client_core_ptr_(in_ptr),
          guard_is_run_ptr_()

    {
      ptr_->request_ = std::move(in_req);
      ptr_->logger_  = in_ptr->ptr_->logger_;
    }
    ~connect_write_read_op()                                       = default;
    // move
    connect_write_read_op(connect_write_read_op&&)                 = default;
    connect_write_read_op& operator=(connect_write_read_op&&)      = default;
    // copy
    connect_write_read_op(const connect_write_read_op&)            = delete;
    connect_write_read_op& operator=(const connect_write_read_op&) = delete;

    void operator()() {
      guard_is_run_ptr_ = std::make_shared<guard_is_run>(*http_client_core_ptr_);
      if (http_client_core_ptr_->stream().socket().is_open()) {
        do_write();
      } else {
        do_resolve();
      }
    }
    void operator()(boost::asio::error::basic_errors) {}

    // async_write async_read 回调
    void operator()(boost::system::error_code ec, std::size_t bytes_transferred) {
      boost::ignore_unused(bytes_transferred);

      if (ec == boost::beast::errc::not_connected || ec == boost::beast::errc::connection_reset ||
          ec == boost::beast::errc::connection_refused || ec == boost::beast::errc::connection_aborted) {
        if (ptr_->retry_count_ > 3) {
          this->complete(false, ec, ptr_->response_);
          return;
        }
        log_info(ptr_->logger_, fmt::format("开始第{}次重试 出现错误 {}", ptr_->retry_count_, ec));
        do_resolve();
        return;
      }
      if (ec) {
        log_info(ptr_->logger_, fmt::format("{}", ec));
        this->complete(false, ec, ptr_->response_);
        return;
      }

      switch (ptr_->state_) {
        case state::write: {
          do_read();
          break;
        }
        case state::read: {
          http_client_core_ptr_->response_header_operator_(http_client_core_ptr_, ptr_->response_);
          this->complete(false, ec, ptr_->response_);
          break;
        }
        default: {
          break;
        }
      }
    }

    // async_connect 回调
    void operator()(boost::system::error_code ec, const boost::asio::ip::tcp::endpoint& endpoint) {
      boost::ignore_unused(endpoint);

      if (ec == boost::beast::errc::not_connected || ec == boost::beast::errc::connection_reset ||
          ec == boost::beast::errc::connection_refused || ec == boost::beast::errc::connection_aborted) {
        if (ptr_->retry_count_ > 3) {
          this->complete(false, ec, ptr_->response_);
          return;
        }
        log_info(ptr_->logger_, fmt::format("开始第{}次重试 出现错误 {}", ptr_->retry_count_, ec));
        do_resolve();
        return;
      }

      if (ec) {
        log_info(ptr_->logger_, fmt::format("{}", ec));
        this->complete(false, ec, ptr_->response_);
        return;
      }

      ptr_->state_       = state::connect;
      ptr_->retry_count_ = 0;
      // log_info(ptr_->logger_, fmt::format("{}", ec));
      if constexpr (use_ssl) {
        http_client_core_ptr_->ptr_->socket_->async_handshake(boost::asio::ssl::stream_base::client, std::move(*this));
      } else {
        do_write();
      }
    }

    // async_handshake 回调
    void operator()(boost::system::error_code ec) {
      if (ec) {
        log_info(ptr_->logger_, fmt::format("{}", ec));
        this->complete(false, ec, ptr_->response_);
        return;
      }
      log_info(ptr_->logger_, "state handshake");
      do_write();
    }

    // async_resolve 回调
    void operator()(boost::system::error_code ec, boost::asio::ip::tcp::resolver::results_type results) {
      if (ec) {
        log_info(ptr_->logger_, fmt::format("{}", ec));
        this->complete(false, ec, ptr_->response_);
        return;
      }
      http_client_core_ptr_->ptr_->resolver_results_ = std::move(results);
      do_connect();
    }

    void do_write() {
      http_client_core_ptr_->expires_after(30s);
      ptr_->state_ = state::write;
      log_info(ptr_->logger_, fmt::format("state {}", ptr_->state_));
      boost::beast::http::async_write(*http_client_core_ptr_->ptr_->socket_, ptr_->request_, std::move(*this));
    }
    void do_read() {
      http_client_core_ptr_->expires_after(30s);
      ptr_->state_ = state::read;
      log_info(ptr_->logger_, fmt::format("state {}", ptr_->state_));
      ptr_->buffer_.clear();
      boost::beast::http::async_read(
          *http_client_core_ptr_->ptr_->socket_, ptr_->buffer_, ptr_->response_, std::move(*this)
      );
    }
    void do_connect() {
      http_client_core_ptr_->expires_after(30s);
      ptr_->state_ = state::resolve;
      ++ptr_->retry_count_;
      log_info(ptr_->logger_, fmt::format("state {}", ptr_->state_));
      http_client_core_ptr_->stream().async_connect(http_client_core_ptr_->ptr_->resolver_results_, std::move(*this));
      // boost::asio::async_connect(socket_.socket(), http_client_core_ptr_->ptr_->resolver_results_, std::move(*this));
    }
    void do_resolve() {
      ptr_->state_ = state::start;
      log_info(ptr_->logger_, fmt::format("state {}", ptr_->state_));
      http_client_core_ptr_->ptr_->resolver_->async_resolve(
          http_client_core_ptr_->server_ip(),
          http_client_core_ptr_->server_port().empty() ? http_client_core_ptr_->ptr_->scheme_
                                                       : http_client_core_ptr_->server_port(),
          std::move(*this)
      );
    }
  };

 public:
  template <typename ResponseType, typename RequestType, typename CompletionHandler>
  auto async_read(RequestType& in_type, CompletionHandler&& in_completion) {
    make_socket();
    request_header_operator_(this, in_type);
    in_type.prepare_payload();
    // std::ostringstream l_oss;
    // l_oss << in_type;
    // default_logger_raw()->log(log_loc(), level::info, l_oss.str());
    auto l_exe       = boost::asio::get_associated_executor(in_completion, g_io_context().get_executor());
    using connect_op = connect_write_read_op<decltype(l_exe), CompletionHandler, ResponseType, RequestType>;
    log_info(ptr_->logger_, fmt::format("{} {}", in_type.target(), fmt::ptr(std::addressof(in_completion))));
    return boost::asio::async_initiate<CompletionHandler, void(boost::system::error_code, ResponseType)>(
        [](auto&& in_completion_, http_client_core* in_client_ptr, const auto& in_executor_, RequestType& in_type) {
          in_client_ptr->ptr_->next_list_.emplace().emplace(connect_op{
              in_client_ptr, std::move(in_type), std::forward<decltype(in_completion_)>(in_completion_), in_executor_
          });
          in_client_ptr->next();
        },
        in_completion, this, l_exe, in_type
    );
  }

 public:
  http_client_core()
      : ptr_(std::make_shared<data_type<use_ssl>>()), request_header_operator_(), response_header_operator_() {
    make_ptr();
  }
  explicit http_client_core(
      std::string in_server_ip, std::string in_server_port_ = std::to_string(doodle_config::http_port)
  )
      : ptr_(std::make_shared<data_type<use_ssl>>()), request_header_operator_(), response_header_operator_() {
    ptr_->server_ip_   = std::move(in_server_ip);
    ptr_->server_port_ = std::move(in_server_port_);

    make_ptr();
  }

  explicit http_client_core(
      boost::asio::ssl::context& in_ctx, std::string in_server_ip,
      std::string in_server_port_ = std::to_string(doodle_config::http_port)
  )
      : ptr_(std::make_shared<data_type<use_ssl>>(in_ctx)), request_header_operator_(), response_header_operator_() {
    ptr_->server_ip_   = std::move(in_server_ip);
    ptr_->server_port_ = std::move(in_server_port_);

    make_ptr();
  }

  ~http_client_core() = default;

  // cancel
  void cancel() {
    boost::system::error_code ec;
    ptr_->socket_->socket().cancel(ec);
    if (ec) {
      ptr_->logger_->log(log_loc(), level::err, "do_close error: {}", ec.message());
    }
  }

  // server_ip
  [[nodiscard]] std::string& server_ip() { return ptr_->server_ip_; }
  [[nodiscard]] const std::string& server_ip() const { return ptr_->server_ip_; }
  void server_ip(std::string in_server_ip) { ptr_->server_ip_ = std::move(in_server_ip); }

  // server_port
  [[nodiscard]] std::string& server_port() { return ptr_->server_port_; }
  [[nodiscard]] const std::string& server_port() const { return ptr_->server_port_; }
  void server_port(std::string in_server_port) { ptr_->server_port_ = std::move(in_server_port); }
  // response_header_operator
  [[nodiscard]] response_header_operator_type& response_header_operator() { return response_header_operator_; }
  [[nodiscard]] const response_header_operator_type& response_header_operator() const {
    return response_header_operator_;
  }
  // request_header_operator
  [[nodiscard]] request_header_operator_type& request_header_operator() { return request_header_operator_; }
  [[nodiscard]] const request_header_operator_type& request_header_operator() const { return request_header_operator_; }

  socket_t::socket_type& socket() { return stream().socket(); }
  socket_t& stream() {
    if constexpr (use_ssl) {
      return boost::beast::get_lowest_layer(*ptr_->socket_);
    } else {
      return *ptr_->socket_;
    }
  }
  resolver_t& resolver() { return *ptr_->resolver_; }
  // logger
  [[nodiscard]] logger_ptr& logger() { return ptr_->logger_; }
  [[nodiscard]] const logger_ptr& logger() const { return ptr_->logger_; }

  inline void restart() { make_ptr(); }

  void do_close() {
    if constexpr (use_ssl) {
      boost::system::error_code ec;
      ptr_->socket_->async_shutdown([ptr = ptr_, logger = ptr_->logger_](boost::system::error_code ec) {
        if (ec) {
          logger->log(log_loc(), level::err, "do_close error: {}", ec.message());
        }
        ptr->socket_.reset();
      });
    } else {
      boost::system::error_code ec;
      ptr_->socket_->socket().shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
      if (ec) {
        ptr_->logger_->log(log_loc(), level::err, "do_close error: {}", ec.message());
      }
      ptr_->socket_.reset();
    }
  }

 private:
  void make_ptr() {
    auto l_s        = boost::asio::make_strand(g_io_context());
    ptr_->resolver_ = std::make_shared<resolver_t>(l_s);
    timer_ptr_      = std::make_shared<timer_t>(l_s);

    boost::urls::url l_url{ptr_->server_ip_};
    if constexpr (use_ssl) {
      auto l_scheme = l_url.scheme();
      ptr_->scheme_ = l_scheme.empty() ? "https" : l_scheme;
    } else {
      ptr_->scheme_ = l_url.scheme().empty() ? "http" : l_url.scheme();
    }

    if (!l_url.host().empty()) ptr_->server_ip_ = l_url.host();
    // 创建socket
    make_socket();
    ptr_->logger_ = g_logger_ctrl().make_log(fmt::format("{}_{}_{}", "http_client_core", l_url.host(), socket()));
  }

  void make_socket() {
    // 首先检查是否已经有 socket
    if (ptr_->socket_) return;

    auto l_s = boost::asio::make_strand(g_io_context());
    if constexpr (use_ssl) {
      boost::urls::url l_url{ptr_->server_ip_};
      auto l_host   = l_url.host();
      ptr_->socket_ = std::make_shared<ssl_socket_t>(timer_ptr_->get_executor(), ptr_->ssl_ctx_);
      boost::beast::ssl_stream<boost::beast::tcp_stream>& l_ssl_stream = *ptr_->socket_;
      l_ssl_stream.set_verify_mode(boost::asio::ssl::verify_none);

      if (!l_host.empty())
        if (!SSL_set_tlsext_host_name(l_ssl_stream.native_handle(), l_host.data())) {
          boost::beast::error_code ec{static_cast<int>(::ERR_get_error()), boost::asio::error::get_ssl_category()};
          default_logger_raw()->log(log_loc(), level::err, "SSL_set_tlsext_host_name: {}", ec.message());
        }

    } else {
      ptr_->socket_ = std::make_shared<socket_t>(timer_ptr_->get_executor());
    }
  }

  void next() {
    if (ptr_->next_list_.empty()) {
      return;
    }
    if (is_run_) return;
    ptr_->next_list_.front().maybe_invoke();
    ptr_->next_list_.pop();
  }

  void expires_after(std::chrono::seconds in_seconds) {
    stream().expires_after(in_seconds);
    timer_ptr_->expires_after(in_seconds);

    timer_ptr_->async_wait([this](const boost::system::error_code& in_ec) {
      if (in_ec) {
        return;
      }
      do_close();
    });
  }
};

class http_client_data_base : public std::enable_shared_from_this<http_client_data_base> {
 public:
  using executor_type  = boost::asio::as_tuple_t<boost::asio::use_awaitable_t<>>;

  using resolver_t     = executor_type::as_default_on_t<boost::asio::ip::tcp::resolver>;
  using resolver_ptr   = std::shared_ptr<resolver_t>;
  using buffer_type    = boost::beast::flat_buffer;

  using timer_t        = boost::asio::steady_timer;
  using timer_ptr_t    = std::shared_ptr<timer_t>;

  using socket_t       = executor_type::as_default_on_t<boost::beast::tcp_stream>;
  using socket_ptr     = std::shared_ptr<socket_t>;

  using ssl_socket_t   = boost::beast::ssl_stream<socket_t>;
  using ssl_socket_ptr = std::shared_ptr<ssl_socket_t>;

  using buffer_type                   = boost::beast::flat_buffer;
 private:
  boost::asio::any_io_executor executor_;

 public:
  http_client_data_base() = default;
  template <typename ExecutorType>
  explicit http_client_data_base(ExecutorType&& in_executor) : executor_(in_executor) {}
  std::string server_ip_{};
  std::string server_port_{};
  logger_ptr logger_{};
  resolver_ptr resolver_{};
  boost::asio::ip::tcp::resolver::results_type resolver_results_;
  buffer_type buffer_{};
  std::variant<socket_ptr, ssl_socket_ptr> socket_{};

  // 定时关闭
  timer_ptr_t timer_ptr_;

  virtual void init(std::string in_server_url, boost::asio::ssl::context* in_ctx = nullptr);

  void expires_after(std::chrono::seconds in_seconds);
  void do_close();

  socket_t& socket();
  ssl_socket_t* ssl_socket();
};

template <typename ResponseBody, typename RequestType>
boost::asio::awaitable<std::tuple<boost::system::error_code, boost::beast::http::response<ResponseBody>>>
read_and_write(
    const std::shared_ptr<http_client_data_base>& in_client_data, const boost::beast::http::request<RequestType>& in_req
) {
  boost::beast::http::response<ResponseBody> l_ret{};
  if (!in_client_data->socket().socket().is_open()) {
    auto [l_e1, l_re] =
        co_await in_client_data->resolver_->async_resolve(in_client_data->server_ip_, in_client_data->server_port_);
    if (l_e1) {
      in_client_data->logger_->log(log_loc(), level::err, "async_resolve error: {}", l_e1.message());
      co_return std::make_tuple(l_e1, l_ret);
    }

    in_client_data->resolver_results_ = l_re;
    auto [l_e2, l_end] = co_await in_client_data->socket().socket().async_connect(*in_client_data->resolver_results_);
    if (l_e2) {
      in_client_data->logger_->log(log_loc(), level::err, "async_connect error: {}", l_e2.message());
      co_return std::make_tuple(l_e2, l_ret);
    }

    if (auto l_ssl = in_client_data->ssl_socket(); l_ssl) {
      auto [l_e3, l_end2] = co_await l_ssl->async_handshake(boost::asio::ssl::stream_base::client);
      if (l_e3) {
        in_client_data->logger_->log(log_loc(), level::err, "async_handshake error: {}", l_e3.message());
        co_return std::make_tuple(l_e3, l_ret);
      }
    }
  }

  auto [l_ew, l_bw] = std::visit(
      [&](auto&& in_socket_ptr) { co_return co_await boost::beast::http::async_write(*in_socket_ptr, in_req); },
      in_client_data->socket_
  );
  if (l_ew) {
    in_client_data->logger_->log(log_loc(), level::err, "async_write error: {}", l_ew.message());
    co_return std::make_tuple(l_ew, l_ret);
  }

  auto [l_er, l_br] = std::visit(
      [&](auto&& in_socket_ptr) {
        co_return co_await boost::beast::http::async_read(*in_socket_ptr, in_client_data->buffer_, l_ret);
      },
      in_client_data->socket_
  );
  if (l_er) {
    in_client_data->logger_->log(log_loc(), level::err, "async_read error: {}", l_er.message());
    co_return std::make_tuple(l_er, l_ret);
  }

  co_return std::make_tuple(boost::system::error_code{}, l_ret);
}

}  // namespace doodle::http::detail

namespace doodle::http {
using http_client_core =
    detail::http_client_core<detail::request_header_operator_base, detail::response_header_operator_base>;
using https_client_core =
    detail::http_client_core<detail::request_header_operator_base, detail::response_header_operator_base, true>;
}  // namespace doodle::http