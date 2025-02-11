//
// Created by TD on 25-1-23.
//

#pragma once

#include <doodle_core/core/co_queue.h>
#include <doodle_core/doodle_core_fwd.h>

#include <doodle_lib/core/http/http_session_data.h>

#include <boost/signals2.hpp>

namespace doodle::socket_io {

enum class socket_io_packet_type : std::uint8_t {
  connect       = 0,
  disconnect    = 1,
  event         = 2,
  ack           = 3,
  connect_error = 4,
  binary_event  = 5,
  binary_ack    = 6
};

struct socket_io_packet {
  socket_io_packet_type type_;
  std::string namespace_;
  std::string id_;
  std::size_t binary_count_{};
  nlohmann::json json_data_{};
  std::vector<std::string> binary_data_{};

  // 从字符串中解析
  static socket_io_packet parse(const std::string& in_str);
};

class event_base {
 protected:
  boost::signals2::signal<void(const std::string&)> event_signal_;

 public:
  virtual ~event_base()                      = default;

  virtual std::string get_last_event() const = 0;
  virtual void event(const socket_io_packet& in_packet);
  virtual void event(std::string in_packet) = 0;

  /// 连接事件
  template <typename Slot>
  auto connect(Slot in_slot) {
    return event_signal_.connect(in_slot);
  }
};

class event_t : public event_base {
 protected:
  std::atomic<std::shared_ptr<std::string>> event_{};

 public:
  std::string get_last_event() const override;
  void event(std::string in_packet) override;
};

class socket_io_websocket_core : public std::enable_shared_from_this<socket_io_websocket_core> {
  logger_ptr logger_;
  std::shared_ptr<boost::beast::websocket::stream<http::tcp_stream_type>> web_stream_;
  std::shared_ptr<awaitable_queue_limitation> write_queue_limitation_;
  http::session_data_ptr handle_;
  uuid sid_{};
  std::shared_ptr<void> sid_lock_{};
  std::string generate_register_reply();

  boost::asio::awaitable<void> async_ping_pong();

 public:
  explicit socket_io_websocket_core(
      http::session_data_ptr in_handle, boost::beast::websocket::stream<http::tcp_stream_type> in_stream
  );

  boost::asio::awaitable<void> run();
  boost::asio::awaitable<void> async_write_websocket(std::string in_data);
  boost::asio::awaitable<void> async_close_websocket();
};

void create_socket_io(
    http::http_route& in_route, const std::shared_ptr<event_base>& in_event, const std::string& in_path = "/socket.io/"
);

}  // namespace doodle::socket_io