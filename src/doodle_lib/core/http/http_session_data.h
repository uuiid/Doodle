//
// Created by TD on 2024/2/20.
//

#pragma once

#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/url.hpp>
namespace doodle::http {
class http_session_data {
 public:
  explicit http_session_data(boost::asio::ip::tcp::socket in_socket)
      : stream_(std::make_unique<boost::beast::tcp_stream>(std::move(in_socket))) {}
  std::unique_ptr<boost::beast::tcp_stream> stream_;
  boost::beast::flat_buffer buffer_;
  boost::url url_;

  inline boost::beast::tcp_stream& operator*() const { return *stream_; }
  inline boost::beast::tcp_stream* operator->() const { return stream_.get(); }

  // copy delete
  http_session_data(const http_session_data&)                = delete;
  http_session_data& operator=(const http_session_data&)     = delete;
  // move
  http_session_data(http_session_data&&) noexcept            = default;
  http_session_data& operator=(http_session_data&&) noexcept = default;
};
}  // namespace doodle::http