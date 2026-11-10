//
// Created by TD on 2026-11-6.
//

#pragma once

#include <doodle_lib/core/face_capture/face_capture_types.h>
#include <doodle_lib/core/face_capture/ws_message.h>
#include <doodle_lib/core/http/http_session_data.h>

#include <doodle_lib/doodle_lib_fwd.h>

#include <boost/asio/any_io_executor.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/beast/core/flat_buffer.hpp>
#include <boost/beast/core/tcp_stream.hpp>
#include <boost/beast/websocket/stream.hpp>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <memory>
#include <vector>

namespace doodle::face_capture {

class capture_session;
class capture_session_holder;

class ws_outlet : public std::enable_shared_from_this<ws_outlet> {
 public:
  using stream_t      = boost::beast::websocket::stream<doodle::http::tcp_stream_type>;
  using executor_type = boost::asio::any_io_executor;

  ws_outlet(
      stream_t in_stream, std::int32_t in_device_index, std::shared_ptr<capture_session_holder> in_holder
  );

  ~ws_outlet();

  ws_outlet(const ws_outlet&)            = delete;
  ws_outlet& operator=(const ws_outlet&) = delete;

  void start();

  void send(std::unique_ptr<ws_message> in_message);

  [[nodiscard]] bool is_attached() const;

  [[nodiscard]] bool idle() const;

  [[nodiscard]] bool pending_frame() const;

  [[nodiscard]] bool stalled() const;

  void mark_closed();

  [[nodiscard]] std::shared_ptr<capture_session> session() const;

 private:
  boost::asio::awaitable<void> run();

  boost::asio::awaitable<void> open_capture();

  boost::asio::awaitable<void> close_capture();

  boost::asio::awaitable<void> read_loop();

  boost::asio::awaitable<void> close_impl();

  boost::asio::awaitable<void> write_message(std::unique_ptr<ws_message> in_message);

  boost::asio::awaitable<void> write_raw(bool in_binary, const std::vector<std::uint8_t>& in_bytes);

  stream_t stream_;
  executor_type exec_;
  std::shared_ptr<capture_session> session_;
  std::shared_ptr<capture_session_holder> holder_;
  boost::beast::flat_buffer read_buffer_;
  std::int32_t device_index_{0};

  std::atomic_bool closed_{false};
  std::atomic_bool writing_{false};

  bool closing_{false};
  std::size_t key_backlog_{0};
  std::chrono::steady_clock::time_point pending_since_{};
};

}  // namespace doodle::face_capture
