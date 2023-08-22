//
// Created by td_main on 2023/8/18.
//
#pragma once
#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_lib/render_farm/detail/basic_json_body.h>

#include <boost/asio.hpp>
#include <boost/beast.hpp>
namespace doodle {
class client {
 private:
  using timer         = boost::asio::system_timer;
  using timer_ptr     = std::shared_ptr<timer>;
  using socket        = boost::beast::tcp_stream;
  using socket_ptr    = std::shared_ptr<socket>;
  using resolver      = boost::asio::ip::tcp::resolver;
  using resolver_ptr  = std::shared_ptr<resolver>;
  using buffer_type   = boost::beast::flat_buffer;
  using response_type = boost::beast::http::response<render_farm::detail::basic_json_body>;
  using request_type  = boost::beast::http::request<render_farm::detail::basic_json_body>;

  struct data_type {
    std::string server_ip_;

    socket_ptr socket_{};
    timer_ptr timer_{};
    resolver_ptr resolver_{};

    buffer_type buffer_;
    response_type response;
    request_type request_;
    std::int32_t connect_count_{};
  };
  std::shared_ptr<data_type> ptr_;

  void make_ptr();

 public:
  explicit client(std::string in_server_ip) : ptr_(std::make_shared<data_type>()) {
    ptr_->server_ip_ = std::move(in_server_ip);
    make_ptr();
  }
  ~client() = default;

  // run
  void run();

  // rest run
  void rest_run();

  // server_ip
  [[nodiscard]] inline std::string& server_ip() { return ptr_->server_ip_; }
  [[nodiscard]] inline const std::string& server_ip() const { return ptr_->server_ip_; }

  // stream

  template <typename CompletionHandler>
  auto async_connect(CompletionHandler&& in_completion) {
    return;
  }

  void list_task();

  inline void server_ip(std::string in_server_ip) { ptr_->server_ip_ = std::move(in_server_ip); }

 private:
  void do_wait();
  void do_resolve();

  // 获取渲染注册机器列表
  void do_get_computer_list();
  // 获取所有的渲染任务
  void do_get_task_list();

  // on resolve
  void on_resolve(boost::system::error_code ec, boost::asio::ip::tcp::resolver::results_type results);

  // on_connect
  void on_connect(boost::system::error_code ec, boost::asio::ip::tcp::endpoint endpoint);
  // on_connect_timeout
  void on_connect_timeout(boost::system::error_code ec);
};

}  // namespace doodle
