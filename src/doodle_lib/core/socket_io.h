//
// Created by TD on 25-1-23.
//

#pragma once

#include <doodle_core/doodle_core_fwd.h>

#include <doodle_lib/core/http/http_session_data.h>
namespace doodle::socket_io {

class event_base {
 public:
  virtual ~event_base() = default;
};

class socket_io {
 protected:
  std::shared_ptr<event_base> event_;

 public:
  explicit socket_io(const std::shared_ptr<event_base>& in_event) : event_(in_event) {}
};

class socket_io_http_get : public socket_io {
 public:
  explicit socket_io_http_get(const std::shared_ptr<event_base>& in_event) : socket_io(in_event) {}
  boost::asio::awaitable<boost::beast::http::message_generator> operator()(http::session_data_ptr in_handle);
};

}  // namespace doodle::socket_io