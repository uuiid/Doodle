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
#include <boost/beast.hpp>

#include <magic_enum.hpp>
namespace doodle::http::detail {

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

class http_client_core : public std::enable_shared_from_this<http_client_core> {
 public:
  using socket_t     = boost::beast::tcp_stream;
  using socket_ptr   = std::shared_ptr<socket_t>;
  using resolver_t   = boost::asio::ip::tcp::resolver;
  using resolver_ptr = std::shared_ptr<resolver_t>;
  using buffer_type  = boost::beast::flat_buffer;

 private:
  using next_fun_t          = std::function<void()>;
  using next_fun_ptr_t      = std::shared_ptr<next_fun_t>;
  using next_fun_weak_ptr_t = std::weak_ptr<next_fun_t>;

  struct next_t {
    virtual void run() = 0;
  };

  struct data_type {
    std::string server_ip_;
    socket_ptr socket_{};
    logger_ptr logger_{};
    resolver_ptr resolver_{};
    boost::asio::ip::tcp::resolver::results_type resolver_results_;

    std::queue<std::shared_ptr<next_t>> next_list_;
  };
  std::shared_ptr<data_type> ptr_;

  bool is_run_ = false;
  // is_run 守卫
  struct guard_is_run {
    http_client_core& http_client_core_;
    explicit guard_is_run(http_client_core& in_core) : http_client_core_(in_core) { http_client_core_.is_run_ = true; }
    ~guard_is_run() {
      http_client_core_.is_run_ = false;
      http_client_core_.next();
    }
  };

  void make_ptr();

  // 通知下一个
  void next();

 public:
  using state = http_client_core_ns::state;

 private:
  template <typename ExecutorType, typename CompletionHandler, typename ResponseType, typename RequestType>
  struct connect_write_read_op : next_t,
                                 boost::beast::async_base<std::decay_t<CompletionHandler>, ExecutorType>,
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
    std::unique_ptr<data_type2> ptr_;
    socket_t& socket_;
    boost::asio::ip::tcp::resolver::results_type& results_;
    http_client_core* http_client_core_ptr_;
    std::shared_ptr<guard_is_run> guard_is_run_ptr_;

    using base_type = boost::beast::async_base<std::decay_t<CompletionHandler>, ExecutorType>;
    explicit connect_write_read_op(
        http_client_core* in_ptr, RequestType&& in_req, CompletionHandler&& in_handler,
        const ExecutorType& in_executor_type_1
    )
        : next_t(),
          base_type(std::move(in_handler), in_executor_type_1),
          boost::asio::coroutine(),
          ptr_(std::make_unique<data_type2>()),
          socket_(in_ptr->stream()),
          results_(in_ptr->ptr_->resolver_results_),
          http_client_core_ptr_(in_ptr),
          guard_is_run_ptr_() {
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

    void run() override {
      guard_is_run_ptr_ = std::make_shared<guard_is_run>(*http_client_core_ptr_);
      if (socket_.socket().is_open()) {
        do_write();
      } else {
        do_resolve();
      }
    }

    void next() {}

    // async_write async_read 回调
    void operator()(boost::system::error_code ec, std::size_t bytes_transferred) {
      boost::ignore_unused(bytes_transferred);

      if (ec == boost::beast::errc::not_connected || ec == boost::beast::errc::connection_reset ||
          ec == boost::beast::errc::connection_refused || ec == boost::beast::errc::connection_aborted) {
        if (ptr_->retry_count_ > 3) {
          this->complete(false, ec, ptr_->response_);
          next();
          return;
        }
        log_info(ptr_->logger_, fmt::format("开始第{}次重试 出现错误 {}", ptr_->retry_count_, ec));
        do_resolve();
        return;
      }
      if (ec) {
        log_info(ptr_->logger_, fmt::format("{}", ec));
        this->complete(false, ec, ptr_->response_);
        next();
        return;
      }

      switch (ptr_->state_) {
        case state::write: {
          do_read();
          break;
        }
        case state::read: {
          this->complete(false, ec, ptr_->response_);
          next();
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
          next();
          return;
        }
        log_info(ptr_->logger_, fmt::format("开始第{}次重试 出现错误 {}", ptr_->retry_count_, ec));
        do_resolve();
        return;
      }

      if (ec) {
        log_info(ptr_->logger_, fmt::format("{}", ec));
        this->complete(false, ec, ptr_->response_);
        next();
        return;
      }

      ptr_->state_       = state::connect;
      ptr_->retry_count_ = 0;
      log_info(ptr_->logger_, fmt::format("{}", ec));
      do_write();
    }
    // async_resolve 回调
    void operator()(boost::system::error_code ec, boost::asio::ip::tcp::resolver::results_type results) {
      if (ec) {
        log_info(ptr_->logger_, fmt::format("{}", ec));
        this->complete(false, ec, ptr_->response_);
        next();
        return;
      }
      http_client_core_ptr_->ptr_->resolver_results_ = std::move(results);
      do_connect();
    }

    void do_write() {
      socket_.expires_after(30s);
      ptr_->state_ = state::write;
      log_info(ptr_->logger_, fmt::format("state {}", ptr_->state_));
      //      auto l_req = ptr_->request_;
      //      boost::beast::async_write(socket_, message_generator_type{std::move(l_req)}, std::move(*this));
      boost::beast::http::async_write(socket_, ptr_->request_, std::move(*this));
    }
    void do_read() {
      socket_.expires_after(30s);
      ptr_->state_ = state::read;
      log_info(ptr_->logger_, fmt::format("state {}", ptr_->state_));
      boost::beast::http::async_read(socket_, ptr_->buffer_, ptr_->response_, std::move(*this));
    }
    void do_connect() {
      socket_.expires_after(30s);
      ptr_->state_ = state::resolve;
      ++ptr_->retry_count_;
      log_info(ptr_->logger_, fmt::format("state {}", ptr_->state_));
      boost::asio::async_connect(socket_.socket(), results_, std::move(*this));
    }
    void do_resolve() {
      ptr_->state_ = state::start;
      log_info(ptr_->logger_, fmt::format("state {}", ptr_->state_));
      http_client_core_ptr_->ptr_->resolver_->async_resolve(
          http_client_core_ptr_->server_ip(), std::to_string(doodle_config::http_port), std::move(*this)
      );
    }
  };

 public:
  template <typename ResponseType, typename RequestType, typename CompletionHandler>
  auto async_read(RequestType& in_type, CompletionHandler&& in_completion) {
    in_type.set(boost::beast::http::field::host, fmt::format("{}:{}", server_ip(), doodle_config::http_port));
    in_type.set(boost::beast::http::field::user_agent, BOOST_BEAST_VERSION_STRING);
    using execution_type = decltype(boost::asio::get_associated_executor(in_completion, g_io_context().get_executor()));
    using connect_op     = connect_write_read_op<execution_type, CompletionHandler, ResponseType, RequestType>;
    this->stream().expires_after(30s);
    log_info(ptr_->logger_, fmt::format("{} {}", in_type.target(), fmt::ptr(std::addressof(in_completion))));
    return boost::asio::async_initiate<CompletionHandler, void(boost::system::error_code, ResponseType)>(
        [](auto&& in_completion_, http_client_core* in_client_ptr, const auto& in_executor_, RequestType& in_type) {
          auto l_h = std::make_shared<connect_op>(
              in_client_ptr, std::move(in_type), std::forward<decltype(in_completion_)>(in_completion_), in_executor_
          );
          in_client_ptr->stream().expires_after(30s);
          in_client_ptr->ptr_->next_list_.emplace(l_h);
          in_client_ptr->next();
        },
        in_completion, this, boost::asio::get_associated_executor(in_completion, g_io_context().get_executor()), in_type
    );
  }

 public:
  explicit http_client_core(std::string in_server_ip) : ptr_(std::make_shared<data_type>()) {
    ptr_->server_ip_ = std::move(in_server_ip);
    make_ptr();
  }
  ~http_client_core();

  // cancel
  void cancel();

  // server_ip
  [[nodiscard]] inline std::string& server_ip() { return ptr_->server_ip_; }
  [[nodiscard]] inline const std::string& server_ip() const { return ptr_->server_ip_; }
  inline void server_ip(std::string in_server_ip) { ptr_->server_ip_ = std::move(in_server_ip); }
  inline socket_t::socket_type& socket() { return ptr_->socket_->socket(); }
  inline socket_t& stream() { return *ptr_->socket_; }
  inline resolver_t& resolver() { return *ptr_->resolver_; }
  // logger
  [[nodiscard]] inline logger_ptr& logger() { return ptr_->logger_; }
  [[nodiscard]] inline const logger_ptr& logger() const { return ptr_->logger_; }

 private:
  void do_close();
};
}  // namespace doodle::http::detail