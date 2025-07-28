//
// Created by TD on 25-1-23.
//

#pragma once

#include <doodle_core/core/co_queue.h>
#include <doodle_core/doodle_core_fwd.h>

#include <doodle_lib/core/http/http_function.h>
#include <doodle_lib/core/http/http_session_data.h>
#include <doodle_lib/core/socket_io/socket_io_ctx.h>

namespace doodle::socket_io {

class socket_io_http_base_fun : public http::http_function {
 protected:
  std::shared_ptr<sid_ctx> sid_ctx_;

 public:
  explicit socket_io_http_base_fun(boost::beast::http::verb in_verb, const std::shared_ptr<sid_ctx>& in_sid_ctx)
      : http_function(in_verb, ucom_t{} / R"(socket\.io)"), sid_ctx_(std::move(in_sid_ctx)) {}
};
class socket_io_http_get : public socket_io_http_base_fun {
  std::string generate_register_reply();
  void init();

 public:
  socket_io_http_get(const std::shared_ptr<sid_ctx>& in_event)
      : socket_io_http_base_fun(boost::beast::http::verb::get, in_event) {}
  boost::asio::awaitable<boost::beast::http::message_generator> callback(http::session_data_ptr in_handle) override;
  [[nodiscard]] bool has_websocket() const override;
  boost::asio::awaitable<void> websocket_callback(
      boost::beast::websocket::stream<http::tcp_stream_type> in_stream, http::session_data_ptr in_handle
  ) override;
};
class socket_io_http_post : public socket_io_http_base_fun {
 public:
  socket_io_http_post(const std::shared_ptr<sid_ctx>& in_event)
      : socket_io_http_base_fun(boost::beast::http::verb::post, in_event) {}

  boost::asio::awaitable<boost::beast::http::message_generator> callback(http::session_data_ptr in_handle) override;
};
class socket_io_http_put : public socket_io_http_base_fun {
 public:
  socket_io_http_put(const std::shared_ptr<sid_ctx>& in_event)
      : socket_io_http_base_fun(boost::beast::http::verb::put, in_event) {}
  boost::asio::awaitable<boost::beast::http::message_generator> callback(http::session_data_ptr in_handle) override;
};

}  // namespace doodle::socket_io