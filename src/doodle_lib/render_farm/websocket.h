//
// Created by td_main on 2023/9/14.
//

#pragma once

#include <boost/beast.hpp>

namespace doodle::render_farm {

class websocket {
 private:
  struct impl {
    explicit impl(boost::beast::websocket::stream<boost::asio::ip::tcp::socket> in_stream)
        : stream_(std::move(in_stream)) {}
    boost::beast::websocket::stream<boost::asio::ip::tcp::socket> stream_;
    boost::beast::flat_buffer buffer_{};
  };

  std::unique_ptr<impl> impl_ptr_{};
  void run();

 public:
  websocket() = default;
  explicit websocket(boost::beast::websocket::stream<boost::asio::ip::tcp::socket> in_stream)
      : impl_ptr_(std::make_unique<impl>(std::move(in_stream))) {}
  ~websocket()                               = default;

  // copy
  websocket(const websocket&)                = delete;
  websocket& operator=(const websocket&)     = delete;
  // move
  websocket(websocket&&) noexcept            = default;
  websocket& operator=(websocket&&) noexcept = default;
};

}  // namespace doodle::render_farm