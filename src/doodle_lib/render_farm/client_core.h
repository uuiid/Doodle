//
// Created by td_main on 2023/8/28.
//

#pragma once
#include <doodle_core/lib_warp/boost_fmt_error.h>

#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_lib/render_farm/detail/basic_json_body.h>

#include <boost/asio.hpp>
#include <boost/beast.hpp>

namespace doodle::detail {
class client_core : public std::enable_shared_from_this<client_core> {
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

  struct data_type {
    std::string server_ip_;
    socket_ptr socket_{};
    logger_ptr logger_{};
    resolver_ptr resolver_{};
    boost::asio::ip::tcp::resolver::results_type resolver_results_;
    boost::asio::any_io_executor executor_;

    next_fun_weak_ptr_t next_;
  };
  std::shared_ptr<data_type> ptr_;

  void make_ptr();
  // on_resolve
  void on_resolve(boost::system::error_code ec, boost::asio::ip::tcp::resolver::results_type results);

 public:
  enum state {
    start,
    resolve,
    connect,
    write,
    read,
  };

 private:
  template <typename ExecutorType, typename CompletionHandler, typename ResponseType, typename RequestType>
  struct connect_write_read_op : boost::beast::async_base<std::decay_t<CompletionHandler>, ExecutorType>,
                                 boost::asio::coroutine {
    struct data_type2 {
      buffer_type buffer_;
      ResponseType response_;
      state state_ = start;
      RequestType request_;

      next_fun_ptr_t next_;
      logger_ptr logger_;
      // 重试次数
      std::size_t retry_count_{};
    };
    std::unique_ptr<data_type2> ptr_;
    socket_t& socket_;
    boost::asio::ip::tcp::resolver::results_type& results_;
    using base_type = boost::beast::async_base<std::decay_t<CompletionHandler>, ExecutorType>;
    explicit connect_write_read_op(
        client_core* in_ptr, RequestType&& in_req, CompletionHandler&& in_handler,
        const ExecutorType& in_executor_type_1
    )
        : base_type(std::move(in_handler), in_executor_type_1),
          socket_(in_ptr->stream()),
          results_(in_ptr->ptr_->resolver_results_),
          ptr_(std::make_unique<data_type2>()) {
      ptr_->request_ = std::move(in_req);
      ptr_->logger_  = in_ptr->ptr_->logger_;
      ptr_->next_    = std::make_shared<next_fun_t>([]() {});
    }
    ~connect_write_read_op()                                       = default;
    // move
    connect_write_read_op(connect_write_read_op&&)                 = default;
    connect_write_read_op& operator=(connect_write_read_op&&)      = default;
    // copy
    connect_write_read_op(const connect_write_read_op&)            = delete;
    connect_write_read_op& operator=(const connect_write_read_op&) = delete;

    void run() {
      if (socket_.socket().is_open()) {
        do_write();
      } else {
        do_connect();
      }
    }
    void next() {
      (*ptr_->next_)();
      ptr_->next_.reset();
    }

    // async_write async_read 回调
    void operator()(boost::system::error_code ec, std::size_t bytes_transferred) {
      boost::ignore_unused(bytes_transferred);

      if (ec == boost::beast::errc::not_connected || ec == boost::beast::errc::connection_reset ||
          ec == boost::beast::errc::connection_refused || ec == boost::beast::errc::connection_aborted) {
        if (ptr_->retry_count_ > 3) {
          ptr_->response_.result(boost::beast::http::status::request_timeout);
          this->complete(false, ec, ptr_->response_);
          next();
          return;
        }
        log_debug(ptr_->logger_, fmt::format("开始第{}次重试 出现错误 {}", ptr_->retry_count_, ec));
        do_connect();
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
          ptr_->response_.result(boost::beast::http::status::request_timeout);
          this->complete(false, ec, ptr_->response_);
          next();
          return;
        }
        log_debug(ptr_->logger_, fmt::format("开始第{}次重试 出现错误 {}", ptr_->retry_count_, ec));
        do_connect();
        return;
      }

      if (ec) {
        log_info(ptr_->logger_, fmt::format("{}", ec));
        this->complete(false, ec, ptr_->response_);
        next();
        return;
      }

      ptr_->state_       = connect;
      ptr_->retry_count_ = 0;
      log_debug(ptr_->logger_, fmt::format("{}", ec));
      do_write();
    }

    void do_write() {
      socket_.expires_after(30s);
      ptr_->state_ = write;
      log_debug(ptr_->logger_, fmt::format("state {}", ptr_->state_));
      //      auto l_req = ptr_->request_;
      //      boost::beast::async_write(socket_, message_generator_type{std::move(l_req)}, std::move(*this));
      boost::beast::http::async_write(socket_, ptr_->request_, std::move(*this));
    }
    void do_read() {
      socket_.expires_after(30s);
      ptr_->state_ = read;
      log_debug(ptr_->logger_, fmt::format("state {}", ptr_->state_));
      boost::beast::http::async_read(socket_, ptr_->buffer_, ptr_->response_, std::move(*this));
    }
    void do_connect() {
      socket_.expires_after(30s);
      ptr_->state_ = state::resolve;
      ++ptr_->retry_count_;
      log_debug(ptr_->logger_, fmt::format("state {}", ptr_->state_));
      boost::asio::async_connect(socket_.socket(), results_, std::move(*this));
    }
  };

 public:
  template <typename ResponseType, typename ExecutorType, typename RequestType, typename CompletionHandler>
  auto async_read(const ExecutorType& in_executor_type, RequestType& in_type, CompletionHandler&& in_completion) {
    in_type.set(boost::beast::http::field::host, fmt::format("{}:50021", server_ip()));
    in_type.set(boost::beast::http::field::user_agent, BOOST_BEAST_VERSION_STRING);
    using connect_op = connect_write_read_op<ExecutorType, CompletionHandler, ResponseType, RequestType>;
    this->stream().expires_after(30s);
    return boost::asio::async_initiate<CompletionHandler, void(boost::system::error_code, ResponseType)>(
        [](auto&& in_completion_, client_core* in_client_ptr, const auto& in_executor_, RequestType& in_type) {
          auto l_h = std::make_shared<connect_op>(
              in_client_ptr, std::move(in_type), std::forward<decltype(in_completion_)>(in_completion_), in_executor_
          );
          in_client_ptr->stream().expires_after(30s);
          bool l_has_next = static_cast<bool>(in_client_ptr->ptr_->next_.lock());
          auto l_next     = l_h->ptr_->next_;
          if (l_has_next) {
            (*in_client_ptr->ptr_->next_.lock()) = [l_h]() {
              log_debug(l_h->ptr_->logger_, "开始下一项");
              l_h->run();
            };
          } else {
            l_h->run();
          }
          in_client_ptr->ptr_->next_ = l_next;
        },
        in_completion, this, in_executor_type, in_type
    );
  }

 public:
  explicit client_core(std::string in_server_ip) : ptr_(std::make_shared<data_type>()) {
    ptr_->server_ip_ = std::move(in_server_ip);
    make_ptr();
  }
  ~client_core();

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

  template <typename ExecutorType, typename CompletionHandler>
  auto async_connect(const ExecutorType& in_executor_type, CompletionHandler&& in_completion) {
    using response_type_1 = boost::beast::http::response<boost::beast::http::empty_body>;
    boost::beast::http::request<boost::beast::http::empty_body> l_request{
        boost::beast::http::verb::get, "/v1/render_farm", 11};
    l_request.keep_alive(true);
    l_request.set(boost::beast::http::field::content_type, "application/json");
    l_request.set(boost::beast::http::field::accept, "text/plain");
    return async_read<response_type_1>(
        in_executor_type, l_request, std::forward<decltype(in_completion)>(in_completion)
    );
  }

 private:
  void do_close();

  //  void on_connect_timeout(boost::system::error_code ec);
};
}  // namespace doodle::detail