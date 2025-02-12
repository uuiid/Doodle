//
// Created by TD on 25-2-12.
//

#pragma once

#include <doodle_core/core/co_queue.h>
#include <doodle_core/doodle_core_fwd.h>

#include <doodle_lib/core/http/http_session_data.h>

#include <boost/signals2.hpp>

namespace doodle::socket_io {
class sid_ctx;
class sid_data;
struct socket_io_packet;

class socket_io_websocket_core : public std::enable_shared_from_this<socket_io_websocket_core> {
  logger_ptr logger_;
  std::shared_ptr<boost::beast::websocket::stream<http::tcp_stream_type>> web_stream_;
  std::shared_ptr<sid_ctx> sid_ctx_{};
  std::shared_ptr<sid_data> sid_data_{};
  std::shared_ptr<awaitable_queue_limitation> write_queue_limitation_;
  http::session_data_ptr handle_;
  std::shared_ptr<void> sid_lock_{};
  std::string generate_register_reply();

  boost::asio::awaitable<void> async_ping_pong();

  boost::asio::awaitable<bool> parse_engine_io(std::string& in_body);
  boost::asio::awaitable<void> parse_socket_io(socket_io_packet& in_body);

 public:
  explicit socket_io_websocket_core(
      http::session_data_ptr in_handle, const std::shared_ptr<sid_ctx>& in_sid_ctx,
      boost::beast::websocket::stream<http::tcp_stream_type> in_stream
  );

  boost::asio::awaitable<void> run();
  boost::asio::awaitable<void> async_write_websocket(std::string in_data);
  boost::asio::awaitable<void> async_close_websocket();
};
}  // namespace doodle::socket_io