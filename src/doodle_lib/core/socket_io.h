//
// Created by TD on 25-1-23.
//

#pragma once

#include <doodle_core/doodle_core_fwd.h>

#include <doodle_lib/core/http/http_session_data.h>
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
 public:
  virtual ~event_base()                                          = default;

  virtual std::optional<socket_io_packet> get_last_event() const = 0;
  virtual void event(const socket_io_packet& in_packet)          = 0;
};

class socket_io {
 protected:
  std::shared_ptr<event_base> event_;

 public:
  explicit socket_io(const std::shared_ptr<event_base>& in_event) : event_(in_event) {}
};

class socket_io_http : public socket_io {
  boost::asio::awaitable<boost::beast::http::message_generator> get_fun_impl(http::session_data_ptr in_handle) const;
  boost::asio::awaitable<boost::beast::http::message_generator> post_fun_impl(http::session_data_ptr in_handle) const;

 public:
  using socket_io::socket_io;
  std::function<boost::asio::awaitable<boost::beast::http::message_generator>(http::session_data_ptr in_handle)>
  get_fun() const {
    return std::bind_front(&socket_io_http::get_fun_impl, this);
  }
  std::function<boost::asio::awaitable<boost::beast::http::message_generator>(http::session_data_ptr in_handle)>
  post_fun() const {
    return std::bind_front(&socket_io_http::post_fun_impl, this);
  }

  void reg(http::http_route& in_route, const std::string& in_path = "socket.io") const;
};

}  // namespace doodle::socket_io