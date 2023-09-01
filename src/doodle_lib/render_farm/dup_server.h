//
// Created by td_main on 2023/9/1.
//

#pragma once
#include <boost/asio.hpp>
#include <boost/beast/core.hpp>

#include <memory>
namespace doodle {

class dup_server : std::enable_shared_from_this<dup_server> {
 public:
  explicit dup_server(boost::asio::io_context& in_io_context, std::uint16_t in_port = 50022)
      : socket_{in_io_context, boost::asio::ip::udp::endpoint(boost::asio::ip::udp::v4(), in_port)},
        signal_set_{in_io_context, SIGINT, SIGTERM} {}
  ~dup_server() = default;

  void run();

 private:
  void do_accept();

  boost::asio::ip::udp::endpoint end_point_;
  boost::asio::ip::udp::socket socket_;
  boost::beast::flat_buffer buffer_;
  boost::asio::signal_set signal_set_;
};

}  // namespace doodle
