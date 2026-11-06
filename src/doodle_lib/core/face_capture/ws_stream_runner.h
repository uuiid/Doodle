//
// Created by TD on 2026-11-6.
//

#pragma once

#include <doodle_lib/core/face_capture/ws_outlet.h>
#include <doodle_lib/core/http/http_session_data.h>

#include <doodle_lib/doodle_lib_fwd.h>

#include <boost/asio/awaitable.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/core/flat_buffer.hpp>
#include <boost/beast/websocket/stream.hpp>

#include <atomic>
#include <cstdint>
#include <memory>

namespace doodle::face_capture {

using ws_stream_t = boost::beast::websocket::stream<http::tcp_stream_type>;

class ws_stream_runner : public std::enable_shared_from_this<ws_stream_runner> {
 public:
  explicit ws_stream_runner(ws_stream_t in_stream, logger_ptr in_logger);

  ~ws_stream_runner();

  ws_stream_runner(const ws_stream_runner&)            = delete;
  ws_stream_runner& operator=(const ws_stream_runner&) = delete;

  void start();

  [[nodiscard]] bool is_open() const;

  void close();

 private:
  boost::asio::awaitable<void> run();

  boost::asio::awaitable<bool> write(bool in_binary, const std::uint8_t* in_data, std::size_t in_size);

  boost::asio::awaitable<void> read_loop();

  boost::asio::awaitable<void> close_impl();

  ws_stream_t stream_;
  logger_ptr logger_;
  boost::asio::strand<boost::asio::io_context::executor_type> strand_;
  std::shared_ptr<ws_outlet> outlet_;
  boost::beast::flat_buffer read_buffer_;
  std::atomic_bool open_{false};
};

}  // namespace doodle::face_capture
