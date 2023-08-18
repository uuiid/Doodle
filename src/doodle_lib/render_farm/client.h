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
  struct data_type {
    std::string server_ip_;
    boost::asio::ip::tcp::socket socket_{g_io_context()};
    boost::asio::system_timer timer_{g_io_context()};
    boost::beast::flat_buffer buffer_;
    boost::beast::http::response<render_farm::detail::basic_json_body> response;
    boost::beast::http::request<render_farm::detail::basic_json_body> request_;
    std::int32_t connect_count_{};
  };
  std::shared_ptr<data_type> ptr_;

 public:
  explicit client(std::string in_server_ip) : ptr_(std::make_shared<data_type>()) {
    ptr_->server_ip_ = std::move(in_server_ip);
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
  // on_connect
  void on_connect(boost::system::error_code ec, boost::asio::ip::tcp::endpoint endpoint);
  // on_connect_timeout
  void on_connect_timeout();
};

}  // namespace doodle
