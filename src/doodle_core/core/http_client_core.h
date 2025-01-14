//
// Created by TD on 2024/3/6.
//

#pragma once
#include <doodle_core/configure/static_value.h>
#include <doodle_core/core/co_queue.h>
#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/lib_warp/boost_fmt_asio.h>
#include <doodle_core/lib_warp/boost_fmt_error.h>
#include <doodle_core/logger/logger.h>

#include <boost/asio.hpp>
#include <boost/asio/ts/netfwd.hpp>
#include <boost/beast.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/url.hpp>

#include <atomic>
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

  using buffer_type      = boost::beast::flat_buffer;

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
    auto [l_e1, l_re] =
        co_await in_client_data->resolver_->async_resolve(in_client_data->server_ip_, in_client_data->server_port_);
    if (l_e1) {
      in_client_data->logger_->log(log_loc(), level::err, "async_resolve error: {}", l_e1.message());
      co_return std::make_tuple(l_e1, l_ret);
    }
    in_client_data->expires_after(std::chrono::seconds{10});

    in_client_data->resolver_results_ = l_re;
    auto [l_e2] = co_await in_client_data->socket().async_connect(*in_client_data->resolver_results_);
    if (l_e2) {
      in_client_data->logger_->log(log_loc(), level::err, "async_connect error: {}", l_e2.message());
      co_return std::make_tuple(l_e2, l_ret);
    }

    if (auto l_ssl = in_client_data->ssl_socket(); l_ssl) {
      auto [l_e3] = co_await l_ssl->async_handshake(boost::asio::ssl::stream_base::client);
      if (l_e3) {
        in_client_data->logger_->log(log_loc(), level::err, "async_handshake error: {}", l_e3.message());
        co_return std::make_tuple(l_e3, l_ret);
      }
    }
    in_client_data->is_open_and_cond_ = true;
  }
  using visit_return_type = boost::asio::async_result<
      http_client_data_base::co_executor_type, void(boost::system::error_code, std::size_t)>::return_type;

  in_client_data->expires_after(30s);
  auto [l_ew, l_bw] = co_await std::visit(
      [in_req_ptr = &in_req](auto&& in_socket_ptr) -> visit_return_type {
        // 此处调整异步堆栈
        auto l_req = in_req_ptr;
        co_return co_await boost::beast::http::async_write(*in_socket_ptr, *l_req);
      },
      in_client_data->socket_
  );

  if (l_ew) {
    in_client_data->logger_->log(log_loc(), level::err, "async_write error: {}", l_ew.message());
    co_return std::make_tuple(l_ew, l_ret);
  }
  in_client_data->expires_after(30s);

  auto [l_er, l_br] = co_await std::visit(
      [in_ret_ptr = &l_ret](auto&& in_socket_ptr) -> visit_return_type {
        // 此处调整异步堆栈
        auto l_ret_ptr = in_ret_ptr;
        buffer_type l_buffer2{};
        co_return co_await boost::beast::http::async_read(*in_socket_ptr, l_buffer2, *l_ret_ptr);
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
} // namespace doodle::http